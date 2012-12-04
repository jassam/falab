#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>


#include "fa_network.h"
#include "fa_trans.h"
#include "fa_udpsrv.h"


#define		TRANS_BUF_SIZE		(10*1024)		//8k is the best buffer size of the trans(experence)
int main()
{
	fa_trans_t *trans;
	
	int  port;
    char hostname[1024];
    int  recv_len;
    int  real_recv_len;
	char buf[TRANS_BUF_SIZE];


	fa_trans_init();				/*initial trans unit*/

    strcpy(hostname, "192.168.20.82");
    port = 1982;

	/*create trans unit*/
	trans = fa_create_trans(fa_create_trans_udpsrv, fa_destroy_trans_udpsrv);

	/*try to open trans*/
	if(trans->open(trans,hostname,port)<0){
        printf("open  fail\n");
        return -1;
	}

    recv_len = TRANS_BUF_SIZE;

    while(1) {

        real_recv_len = trans->recv(trans, buf, recv_len);
        printf("-->want %d, recv %d bytes\n", recv_len, real_recv_len);

        if(real_recv_len< 0){
            printf("recv fail\n");
            continue;
        }
    }
	

    fa_trans_uninit();

	return 0;


}
