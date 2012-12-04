#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "fa_network.h"
#include "fa_trans.h"

#include "fa_udp.h"



#define		TRANS_BUF_SIZE		(8*1024)		//8k is the best buffer size of the trans(experence)
int trans_file(char *src_file,char *dest_url)
{
    FILE *fp;
	/*int fd;*/
	int file_len;
	char buf[TRANS_BUF_SIZE];

	fa_trans_t *trans;
	int send_len;
	int read_len;

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
	trans = fa_create_trans(fa_create_trans_udp, fa_destroy_trans_udp);

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
			goto fail;
		}
		else {
			if(read_len < TRANS_BUF_SIZE) {

			}else {
				send_len = trans->send(trans,buf,read_len);
                printf("-->send %d bytes\n", send_len);

				if(send_len < 0){
                    printf("send fail\n");
					goto fail;
				}
				
				if(send_len < read_len) {
                    printf("send_len < read_len\n");
                }

			}

		}

	}

	fa_destroy_trans(trans);
	return 0;

fail:
	fa_destroy_trans(trans);
	return -1;

}


int main()
{

	/*char *sfile = "/home/luolongzhi/Project/ta/xs.wav";*/
	char *sfile = "xs.wav";
    char *dest_url = "udp://192.168.20.82:1982";
	/*char *dest_url = "udp://127.0.0.1:1982";*/
	
	fa_trans_init();				/*initial trans unit*/
	trans_file(sfile,dest_url);	
    fa_trans_uninit();


	return 0;


}
