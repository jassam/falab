/*
  falab - free algorithm lab 
  Copyright (C) 2012 luolongzhi 罗龙智 (Chengdu, China)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.


  filename: fa_tcpsrv.c 
  version : v1.0.0
  time    : 2012/12/5  
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/

#include "fa_tcpsrv.h"
#include "fa_network.h"
#include "fa_print.h"

#ifdef __GNUC__
/*#include <poll.h>*/
#endif

#include <fcntl.h>
#include <errno.h>



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


static int tcp_send(int fd, char *buf , int size)
{
    int ret, size1, fd_max, len;
    fd_set wfds;
    struct timeval tv;

    size1 = size;
    while (size > 0) {
        fd_max = fd;
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        ret = select(fd_max + 1, NULL, &wfds, NULL, &tv);
        if (ret > 0 && FD_ISSET(fd, &wfds)) {
            len = send(fd, buf, size, 0);
            if (len < 0) {
                if (fa_neterrno() != FA_NETERROR(EINTR) &&
                    fa_neterrno() != FA_NETERROR(EAGAIN))
                    return -1;//im_neterrno();
                continue;
            }
            size -= len;
            buf += len;
        } else if (ret < 0) {
            if (fa_neterrno() == FA_NETERROR(EINTR))
                continue;
            return -1;
        }
    }
    return size1 - size;
}


static int tcp_recv(int fd, char *buf , int size)
{
    int len, fd_max, ret;
    fd_set rfds;
    struct timeval tv;

    for (;;) {
        fd_max = fd;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;	//100ms check once
        ret = select(fd_max + 1, &rfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(fd, &rfds)) {
            len = recv(fd, buf, size, 0);
            if (len < 0) {
                if (fa_neterrno() != FA_NETERROR(EINTR) &&
                    fa_neterrno() != FA_NETERROR(EAGAIN))
                    return fa_neterrno();
            } else return len;
        } else if (ret < 0) {
            if (fa_neterrno() == FA_NETERROR(EINTR))
                continue;
            return -1;
        }
    }
}


int fa_recvfrom_client(int fd, char *buf, int size)
{
    int ret;

    ret = tcp_recv(fd, buf, size);

    return ret;
}

int fa_sendto_client(int fd, char *buf, int size)
{
    int ret;

    ret = tcp_send(fd, buf, size);

    return ret;
}

typedef struct _fa_tcpsrv_t{

    int client_nb;					//the number of the clients which are in connect state 
    int stop_srv ;
    int is_running;
    int heart_hop ;
    struct sockaddr_in local_addr;

    int (* handle_new_connection)(int socket_fd);
    int (* handle_connection_read) (int socket_fd);

} fa_tcpsrv_t;


unsigned long fa_tcpsrv_init(int (* handle_new_connection_callback)(int socket_fd), 
                             int (* handle_connection_read_callback)(int socket_fd))
{

    fa_tcpsrv_t *f = NULL;

    f = (fa_tcpsrv_t *)malloc(sizeof(fa_tcpsrv_t));
    memset(f, 0, sizeof(fa_tcpsrv_t));

    f->client_nb  = 0;
    f->stop_srv   = 0;
    f->is_running = 0;
    f->heart_hop  = 0;
    f->handle_new_connection = NULL;
    f->handle_connection_read = NULL;

    if (handle_new_connection_callback)
        f->handle_new_connection = handle_new_connection_callback;

    if (handle_connection_read_callback)
        f->handle_connection_read = handle_connection_read_callback;

    return (unsigned long)f;
}

void fa_tcpsrv_uninit(unsigned long handle)
{
    fa_tcpsrv_t *f = (fa_tcpsrv_t *)handle;


    if (f) {
        f->client_nb  = 0;
        f->stop_srv   = 0;
        f->is_running = 0;
        f->heart_hop  = 0;

        f->handle_new_connection = NULL;
        f->handle_connection_read = NULL;

        free(f);
        f = NULL;
    }

}

int fa_tcpsrv_start(unsigned long handle, const char *hostname, const int port)
{
    fa_tcpsrv_t *f = (fa_tcpsrv_t *)handle;
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

    f->is_running = 0;

	/*set the poll table which control the fd file
	 *use the first element of the poll_table to be the listen port ,poll_table[0]
	 *all of the fd num is the 1+CLENT_MAX_NUM(for listen port need one)*/
    if(!(poll_client = fa_malloc((CLIENT_MAX_NUM+ 1)*sizeof(*poll_client)))) {
        FA_PRINT("FAIL: %s %d, [err at: %s-%d]\n", "Impossible to allocate a poll table handling connections num=. ", CLIENT_MAX_NUM, __FILE__, __LINE__);
        goto fail;
    }

	/*set the local host address info*/
	f->local_addr.sin_family = AF_INET;
	f->local_addr.sin_port = htons(port);
	resolve_host(&f->local_addr.sin_addr, hostname);

	/*return a server_fd for the listen port
	 *be careful that we should close socket when we quit the server to free the memory*/
    if (f->local_addr.sin_port) {
        server_fd = socket_open_listen(&f->local_addr);
        if (server_fd < 0)
            goto fail;
    }
        
	/*the first element of poll_table used to be the listen port,reset the poll_entry*/
    if (server_fd) {
        poll_client[0].fd = server_fd;
        poll_client[0].events = POLLIN;
    }

	f->client_nb = 0;	// set the number of the client to zero

	FA_PRINT("======================== TCP Server start! ============================\n");
	/*loop for dectecting the request connection and query each effect fd and process the connection*/
    for(;;) {

        f->is_running = 1;
        f->heart_hop++;
        if (f->heart_hop < 0)
            f->heart_hop = 0;

        if (f->stop_srv) {
            FA_PRINT("========================== TCP Server is stopped =======================\n");
            break;
        }

        delay = 1000;

        /* wait for an event on one connection. We poll at least every
           second to handle timeouts */
        do {
            client_ready_nb = poll(poll_client, f->client_nb+1, delay);
            if (client_ready_nb < 0 && fa_neterrno() != FA_NETERROR(EAGAIN) &&
                fa_neterrno() != FA_NETERROR(EINTR)) {
                FA_PRINT("poll error, errno=%d\n", fa_neterrno());
                /*goto fail;*/
                continue;
            }
        } while (client_ready_nb < 0);

		/*test the listen fd, check if can be read(POLLRDNORM) )*/
		if(poll_client[0].revents & POLLRDNORM) {
			addr_len = sizeof(from_addr);
			connect_fd = accept(server_fd,(struct sockaddr*)&from_addr, &addr_len);
			if (connect_fd < 0) {
                FA_PRINT("FAIL: %s %d, [err at: %s-%d]\n", "connect_fd err when accept fd ", connect_fd,  __FILE__, __LINE__);
                /*goto fail;*/
                continue;
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
			if(i > f->client_nb)
				f->client_nb = i;
			
			FA_PRINT("connect from %s is accept, current client_nb=%d \n",inet_ntoa(from_addr.sin_addr), f->client_nb);
			/*handle_new_connection(connect_fd);*/
            if (f->handle_new_connection)
                f->handle_new_connection(connect_fd);
            else 
                printf("detect new connection, but do nothing\n");

			if(--client_ready_nb <= 0)
				continue;
		}

		/* check all clients for data */
		for(i = 1 ; i <= f->client_nb ; i++) {
			if((socket_fd = poll_client[i].fd)==0)
				continue;

			if(poll_client[i].revents & (POLLRDNORM | POLLERR)) {
                ret = f->handle_connection_read(socket_fd);

				if(ret < 0){
					if(fa_neterrno() == FA_NETERROR(ECONNRESET)){
						/*connection reset by client */
						closesocket(socket_fd);
						poll_client[i].fd = 0;
                        f->client_nb--;
					}else {
                        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", "unknown error", __FILE__, __LINE__);
					}
				}else if(ret == 0) {
					/*connection closed by client */
					closesocket(socket_fd);
					poll_client[i].fd = 0;
                    f->client_nb--;
				}

				if(--client_ready_nb <= 0)
					break;
			}
		}
		
    }

    f->is_running = 0;
    f->heart_hop  = 0;

    return 0;

fail:
    f->is_running = 0;
    f->heart_hop  = 0;
    FA_PRINT("========================== TCP Server Exit abnormal =======================\n");

    return -1;
	
}


int fa_tcpsrv_stop(unsigned long handle)
{
    fa_tcpsrv_t *f = (fa_tcpsrv_t *)handle;

    if (f->is_running) {
        f->stop_srv = 1;
    } else {
        FA_PRINT("========================== TCP Server is not running, no need stop =======================\n");
    }

    return 0;
}


void fa_tcpsrv_getstatus(unsigned long handle, int *client_nb, int *is_running, int *heart_hop)
{
    fa_tcpsrv_t *f = (fa_tcpsrv_t *)handle;

    *client_nb  = f->client_nb;
    *is_running = f->is_running;
    *heart_hop  = f->heart_hop;

}


