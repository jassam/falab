#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>


#include "fa_tcpsrv.h"
#include "fa_print.h"


typedef struct _client_t {
    int socket_fd;
    /*int file_fd;*/
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

#ifdef WIN32
    _sprintf(file_cnt_num, "%d", client_cnt);
#else 
    sprintf(file_cnt_num, "%d", client_cnt);
#endif

	strcat(dest_file,file_cnt_num);
	strcat(dest_file,".wav");

	/*file_fd[file_open_cnt++] = open(dest_file,O_CREAT|O_WRONLY|O_BINARY);*/
	/*file_fd[file_open_cnt++] = open(dest_file,O_CREAT|O_WRONLY);*/
    client[client_cnt].socket_fd = socket_fd;
    /*client[client_cnt].file_fd   = open(dest_file,O_CREAT|O_WRONLY);*/
    client[client_cnt].file_fp   = fopen(dest_file, "wb");
    client_cnt++;

    return 0;

}


static int handle_connection_read(int socket_fd)
{
	int ret;
	char buf[1024];
	int len;
    int i;
    FILE *fp;

    for (i = 0; i < client_cnt; i++) {
        if (client[i].socket_fd == socket_fd) {
            fp = client[i].file_fp;
        }
    }

	len = recv(socket_fd, buf, 1024, 0);

    if(len > 0) {
        /*write(fd, buf, len);*/
        fwrite(buf, len , 1, fp);
        printf("-->socket_fd=%d, file_fd = %d, write %d byts\n", socket_fd, fp,  len);
    }

	return len;
}


int main()
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


    memset(client, 0, sizeof(client_t)*MAX_CNT);

    fa_tcpsrv_init(handle_new_connection, handle_connection_read);

	fa_tcpsrv_start("192.168.20.82", 1982);

    fa_tcpsrv_uninit();

	return 0;
}
