#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

/*this is an example for useage of fa_trans*/

#include "fa_network.h"
#include "fa_trans.h"
#include "fa_tcp.h"
#include "fa_print.h"



#define		TRANS_BUF_SIZE		(7*1024)		//8k is the best buffer size of the trans(experence)
int trans_file(char *src_file,char *dest_url)
{
    FILE *fp;
	/*int fd;*/
	int file_len;
	char buf[TRANS_BUF_SIZE];
    char buf1[128];

	fa_trans_t *trans;
	int send_len;
	int read_len;
    int recv_len;

	int port;
    char hostname[1024],proto[1024],path[1024];

    memset(hostname, 0, 1024);
    memset(proto, 0 , 1024);
    memset(path, 0, 1024);
    fa_url_split(proto, sizeof(proto), NULL, 0, hostname, sizeof(hostname),
                 &port, path, sizeof(path), dest_url);

	/*fd = open(src_file,O_RDONLY|O_BINARY);*/
	fp = fopen(src_file, "rb");

	if(fp == NULL) {
        printf("srcfile open fail\n");
		goto fail;
	}
    fseek(fp, 0, SEEK_END);
    file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

	/*create trans unit*/
	trans = fa_create_trans(fa_create_trans_tcp, fa_destroy_trans_tcp);

	/*try to open trans*/
	if(trans->open(trans,hostname,port)<0){
        printf("open connect fail\n");
		goto fail;
	}

	for(;;) {
		/*read_len = read(fd,buf,TRANS_BUF_SIZE);*/
        read_len = fread(buf, 1, TRANS_BUF_SIZE, fp); 

		if(read_len < TRANS_BUF_SIZE){
            printf("read_len < TRANS_BUF_SIZE\n");
#if 0
			goto fail;
#else 
            fseek(fp, 0, SEEK_SET);
            continue;
#endif
		}
		else {
            send_len = trans->send(trans,buf,read_len);
            printf("-->send %d bytes\n", send_len);

            memset(buf1, 0, 128);
            recv_len = trans->recv(trans,buf1, 128);
            printf("recv reply %d bytes, info=%s\n", recv_len, buf1);

            if(send_len < 0){
                printf("send fail\n");
                /*goto fail;*/
            }
            
            if(send_len < read_len) {
                printf("send_len < read_len\n");
            }

		}

	}

    printf("finish\n");
	fa_destroy_trans(trans);
	return 0;

fail:
	fa_destroy_trans(trans);
	return -1;

}

/*#define USE_LOGFILE*/

int main()
{

	char *sfile = "xs.wav";
    /*char *dest_url = "tcp://192.168.20.38:1982";*/
    char *dest_url = "tcp://192.168.20.82:1982";

#ifdef USE_LOGFILE
#ifdef WIN32 
    FA_PRINT_INIT(FA_PRINT_ENABLE,
                  FA_PRINT_FILE_ENABLE,FA_PRINT_STDOUT_ENABLE,FA_PRINT_STDERR_ENABLE,
                  FA_PRINT_PID_ENABLE,
                 ".\\log_tcptrans\\", "tcptranslog", "TCPTRANS", 10);
#else 
    FA_PRINT_INIT(FA_PRINT_ENABLE,
                  FA_PRINT_FILE_ENABLE,FA_PRINT_STDOUT_ENABLE,FA_PRINT_STDERR_ENABLE,
                  FA_PRINT_PID_ENABLE,
                 "./log_tcptrans/", "tcptranslog", "TCPTRANS", 10);
#endif
#endif 

    if (fa_network_init()) {
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_SYS_IO, __FILE__, __LINE__);
        return -1;
	}

#ifdef __GNUC__
    fa_sigpipe_init(NULL);
#endif


	trans_file(sfile,dest_url);	

	fa_network_close();

	return 0;


}
