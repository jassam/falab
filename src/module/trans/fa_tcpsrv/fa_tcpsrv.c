#include "fa_tcpsrv.h"
#include "fa_network.h"
#include "fa_trans.h"
#include "fa_print.h"

#ifdef __GNUC__
/*#include <poll.h>*/
#endif

#include <fcntl.h>
#include <errno.h>

#define	CLIENT_MAX_NUM	2000

static struct sockaddr_in local_addr;

/* resolve host with also IP address parsing */
static int resolve_host(struct in_addr *sin_addr, const char *hostname)
{

    if (!fa_inet_aton(hostname, sin_addr)) {
#ifdef WIN32
        struct addrinfo *ai, *cur;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        if (getaddrinfo(hostname, NULL, &hints, &ai))
            return -1;
        /* getaddrinfo returns a linked list of addrinfo structs.
         * Even if we set ai_family = AF_INET above, make sure
         * that the returned one actually is of the correct type. */
        for (cur = ai; cur; cur = cur->ai_next) {
            if (cur->ai_family == AF_INET) {
                *sin_addr = ((struct sockaddr_in *)cur->ai_addr)->sin_addr;
                freeaddrinfo(ai);
                return 0;
            }
        }
        freeaddrinfo(ai);
        return -1;
#else
        struct hostent *hp;
        hp = gethostbyname(hostname);
        if (!hp)
            return -1;
        memcpy(sin_addr, hp->h_addr_list[0], sizeof(struct in_addr));
#endif
    }
    return 0;
}


/* open a listening socket */
static int socket_open_listen(struct sockaddr_in *my_addr)
{
    int server_fd, tmp;

    server_fd = socket(AF_INET,SOCK_STREAM,0);
    if (server_fd < 0) {
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", "socket create error", __FILE__, __LINE__);
        return -1;
    }

    tmp = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

    if (bind (server_fd, (struct sockaddr *) my_addr, sizeof (*my_addr)) < 0) {
        char bindmsg[32];
        snprintf(bindmsg, sizeof(bindmsg), "bind(port %d)", ntohs(my_addr->sin_port));
//        perror (bindmsg);
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", bindmsg, __FILE__, __LINE__);
        closesocket(server_fd);
        return -1;
    }

    if (listen (server_fd, 5) < 0) {
  //      perror ("listen");
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", "listen error", __FILE__, __LINE__);
        closesocket(server_fd);
        return -1;
    }
    fa_socket_nonblock(server_fd, 1);

    return server_fd;
}


int send_repl_client_len(int send_sock,char *msg,int len) {
	if (send(send_sock, msg, len,0) < 0) { 
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", "send error", __FILE__, __LINE__);
		closesocket(send_sock);
	}
	return 0;
}


int send_repl_client(int send_sock,char *msg) {
	send_repl_client_len(send_sock,msg,strlen(msg));
	return 0;
}

/**
 * Send single reply to the additional transfer socket, given the raply and its length.
 */
int send_repl_len(int send_sock,char *msg,int len) {
	if (send(send_sock, msg, len,0) < 0) { 
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", "send repl error", __FILE__, __LINE__);
		closesocket(send_sock);
	}
	return 0;
}


typedef struct _fa_tcpsrv_t{
    int (* handle_new_connection)(int socket_fd);
    int (* handle_connection_read) (int socket_fd);
} fa_tcpsrv_t;


static fa_tcpsrv_t tcpsrv;
static int client_nb  = 0;					//the number of the clients which are in connect state 
static int stop_srv   = 0;
static int is_running = 0;
static int heart_hop  = 0;

int fa_tcpsrv_init(int (* handle_new_connection_callback)(int socket_fd), 
                   int (* handle_connection_read_callback)(int socket_fd))
{
#ifdef WIN32 
    FA_PRINT_INIT(FA_PRINT_ENABLE,
                  /*FA_PRINT_FILE_ENABLE,FA_PRINT_STDOUT_DISABLE,FA_PRINT_STDERR_DISABLE,*/
                  FA_PRINT_FILE_ENABLE,FA_PRINT_STDOUT_ENABLE,FA_PRINT_STDERR_ENABLE,
                  FA_PRINT_PID_ENABLE,
                 ".\\log_tcpsrv\\", "tcpsrvlog", "TCPSRV", 10);
#else 
    FA_PRINT_INIT(FA_PRINT_ENABLE,
                  /*FA_PRINT_FILE_ENABLE,FA_PRINT_STDOUT_DISABLE,FA_PRINT_STDERR_DISABLE,*/
                  FA_PRINT_FILE_ENABLE,FA_PRINT_STDOUT_ENABLE,FA_PRINT_STDERR_ENABLE,
                  FA_PRINT_PID_ENABLE,
                 "./log_tcpsrv/", "tcpsrvlog", "TCPSRV", 10);
#endif


	/*init the network*/
    if (fa_network_init()) {
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_SYS_IO, __FILE__, __LINE__);
        return -1;
	}

    client_nb  = 0;
    stop_srv   = 0;
    is_running = 0;
    heart_hop  = 0;
    tcpsrv.handle_new_connection = NULL;
    tcpsrv.handle_connection_read = NULL;

    if (handle_new_connection_callback)
        tcpsrv.handle_new_connection = handle_new_connection_callback;

    if (handle_connection_read_callback)
        tcpsrv.handle_connection_read = handle_connection_read_callback;

    return 0;
}

int fa_tcpsrv_uninit()
{
	fa_network_close();

    client_nb  = 0;
    stop_srv   = 0;
    is_running = 0;
    heart_hop  = 0;

    tcpsrv.handle_new_connection = NULL;
    tcpsrv.handle_connection_read = NULL;

    return 0;
}

int fa_tcpsrv_start(const char *hostname, const int port)
{
	int i;
	int ret;
    int server_fd = 0;
	int connect_fd = 0 , socket_fd = 0;
	
    int delay, delay1;
    struct pollfd *poll_client;		//the client and the listen fd,max is (CLENT_MAX_NUM+1)
	int client_ready_nb;			//the number return by the poll function, means fd ready num(read or write,can access)
	struct sockaddr_in from_addr;
	int addr_len;

//    HTTPContext *c, *c_next;

    is_running = 0;

	/*set the poll table which control the fd file
	 *use the first element of the poll_table to be the listen port ,poll_table[0]
	 *all of the fd num is the 1+CLENT_MAX_NUM(for listen port need one)*/
    if(!(poll_client = fa_malloc((CLIENT_MAX_NUM+ 1)*sizeof(*poll_client)))) {
        FA_PRINT("FAIL: %s %d, [err at: %s-%d]\n", "Impossible to allocate a poll table handling connections num=. ", CLIENT_MAX_NUM, __FILE__, __LINE__);
        goto fail;
    }

	/*set the local host address info*/
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(port);
	resolve_host(&local_addr.sin_addr, hostname);

	/*return a server_fd for the listen port
	 *be careful that we should close socket when we quit the server to free the memory*/
    if (local_addr.sin_port) {
        server_fd = socket_open_listen(&local_addr);
        if (server_fd < 0)
            goto fail;
    }
        
	/*the first element of poll_table used to be the listen port,reset the poll_entry*/
    if (server_fd) {
        poll_client[0].fd = server_fd;
        poll_client[0].events = POLLIN;
    }

	client_nb = 0;	// set the number of the client to zero

	FA_PRINT("======================== TCP Server start! ============================\n");
	/*loop for dectecting the request connection and query each effect fd and process the connection*/
    for(;;) {

        is_running = 1;
        heart_hop++;
        if (heart_hop < 0)
            heart_hop = 0;

        if (stop_srv) {
            FA_PRINT("========================== TCP Server is stopped =======================n");
            break;
        }

        delay = 1000;

        /* wait for an event on one connection. We poll at least every
           second to handle timeouts */
        do {
            client_ready_nb = poll(poll_client, client_nb+1, delay);
            if (client_ready_nb < 0 && fa_neterrno() != FA_NETERROR(EAGAIN) &&
                fa_neterrno() != FA_NETERROR(EINTR))
                goto fail;
        } while (client_ready_nb < 0);

		/*test the listen fd, check if can be read(POLLRDNORM) )*/
		if(poll_client[0].revents & POLLRDNORM) {
			addr_len = sizeof(from_addr);
			connect_fd = accept(server_fd,(struct sockaddr*)&from_addr, &addr_len);
			if (connect_fd < 0) {
                FA_PRINT("FAIL: %s %d, [err at: %s-%d]\n", "connect_fd err when accept fd ", connect_fd,  __FILE__, __LINE__);
                goto fail;
			}
			fa_socket_nonblock(connect_fd, 1);

			/*form 1 cause: 0 is used to be listen*/
			for(i = 1 ; i <= CLIENT_MAX_NUM ; i++) {
				if(!poll_client[i].fd) {
					poll_client[i].fd = connect_fd;
					break;
				}
			}
			
			/*all the client are busy , no client can be allocate for the new connection*/
			if(i == (CLIENT_MAX_NUM+1)) {
                FA_PRINT("FAIL: %s , [err at: %s-%d]\n", "too many clients", __FILE__, __LINE__);
				closesocket(connect_fd);
			}

			poll_client[i].events = POLLRDNORM;
			/*update the client number after accept the new connection*/
			if(i > client_nb)
				client_nb = i;
			
			FA_PRINT("connect from %s is accept \n",inet_ntoa(from_addr.sin_addr));
			/*handle_new_connection(connect_fd);*/
            if (tcpsrv.handle_new_connection)
                tcpsrv.handle_new_connection(connect_fd);
            else 
                printf("detect new connection, but do nothing\n");

			if(--client_ready_nb <= 0)
				continue;
		}

		/* check all clients for data */
		for(i = 1 ; i <= client_nb ; i++) {
			if((socket_fd = poll_client[i].fd)==0)
				continue;

			if(poll_client[i].revents & (POLLRDNORM | POLLERR)) {
                ret = tcpsrv.handle_connection_read(socket_fd);

				if(ret < 0){
					if(fa_neterrno() == FA_NETERROR(ECONNRESET)){
						/*connection reset by client */
						closesocket(socket_fd);
						poll_client[i].fd = 0;
					}else {
                        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", "unknown error", __FILE__, __LINE__);
					}
				}else if(ret == 0) {
					/*connection closed by client */
					closesocket(socket_fd);
					poll_client[i].fd = 0;
				}

				if(--client_ready_nb <= 0)
					break;
			}
		}
		
    }

    is_running = 0;
    heart_hop  = 0;

    return 0;

fail:
    is_running = 0;
    heart_hop  = 0;
    FA_PRINT("========================== TCP Server Exit abnormal =======================n");

    return -1;
	
}


int fa_tcpsrv_stop()
{
    if (is_running) {
        stop_srv = 1;
    } else {
        FA_PRINT("========================== TCP Server is not running, no need stop =======================n");
    }

    return 0;
}




