#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>


#include "fa_network.h"
#include "fa_trans.h"
#include "fa_tcpsrv.h"
#include "fa_print.h"


typedef struct _client_t {
    int socket_fd;
    FILE *file_fp;
} client_t;

#define MAX_CNT 20

int client_cnt = 0;
client_t client[MAX_CNT];

static int handle_new_connection(int socket_fd)
{
    int i;
	char dest_file[15];// = "recv";
	char file_cnt_num[5];


    for (i = 0; i < client_cnt; i++) {
        if (client[i].socket_fd == socket_fd) {
            return 0;
        }
    }

    printf("new connection fd=%d\n\n", socket_fd);
	memset(dest_file,0,sizeof(dest_file));
	strcpy(dest_file,"recv");
	memset(file_cnt_num,0,sizeof(file_cnt_num));

    sprintf(file_cnt_num, "%d", client_cnt);

	strcat(dest_file,file_cnt_num);
	strcat(dest_file,".wav");

    client[client_cnt].socket_fd = socket_fd;
    client[client_cnt].file_fp   = fopen(dest_file, "wb");
    client_cnt++;

    return 0;

}


static int handle_connection_read(int socket_fd)
{
	int ret;
	char buf[1024*60];
    char buf1[128];
	int len;
    int i;
    FILE *fp;

    for (i = 0; i < client_cnt; i++) {
        if (client[i].socket_fd == socket_fd) {
            fp = client[i].file_fp;
        }
    }

    len = fa_recvfrom_client(socket_fd, buf, 7*1024);

    if(len > 0) {
        fwrite(buf, len , 1, fp);
        printf("-->socket_fd=%d, file_fd = %d, write %d byts\n", socket_fd, fp,  len);
    }

    sprintf(buf1, "recv %d bytes\n", len);
    fa_sendto_client(socket_fd, buf1, strlen(buf1));

	return len;
}

#define USE_LOGFILE

int main()
{
    unsigned long h_tcpsrv;

#ifdef USE_LOGFILE
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
#endif

	/*init the network*/
    if (fa_network_init()) {
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_SYS_IO, __FILE__, __LINE__);
        return -1;
	}

#ifdef __GNUC__
    fa_sigpipe_init(NULL);
#endif

    /*clean the client number*/
    memset(client, 0, sizeof(client_t)*MAX_CNT);

    h_tcpsrv = fa_tcpsrv_init(handle_new_connection, handle_connection_read);

    /*fa_tcpsrv_start(h_tcpsrv, "192.168.20.38", 1982);*/
    fa_tcpsrv_start(h_tcpsrv, "192.168.20.82", 1982);

    fa_tcpsrv_uninit(h_tcpsrv);

    /*close network*/
	fa_network_close();

	return 0;
}
