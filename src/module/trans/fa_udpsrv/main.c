#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>


#include "fa_network.h"
#include "fa_trans.h"
#include "fa_udpsrv.h"
#include "fa_print.h"


#define		TRANS_BUF_SIZE		(6*1024)		//8k is the best buffer size of the trans(experence)

/*#define USE_LOGFILE*/

int main()
{
	fa_trans_t *trans;
	
	int  port;
    char hostname[1024];
    int  recv_len;
    int  real_recv_len;
	char buf[TRANS_BUF_SIZE];

	
#ifdef USE_LOGFILE
#ifdef WIN32 
    FA_PRINT_INIT(FA_PRINT_ENABLE,
                  FA_PRINT_FILE_ENABLE,FA_PRINT_STDOUT_ENABLE,FA_PRINT_STDERR_ENABLE,
                  FA_PRINT_PID_ENABLE,
                 ".\\log_udpsrv\\", "udpsrvlog", "UDPSRV", 10);
#else 
    FA_PRINT_INIT(FA_PRINT_ENABLE,
                  FA_PRINT_FILE_ENABLE,FA_PRINT_STDOUT_ENABLE,FA_PRINT_STDERR_ENABLE,
                  FA_PRINT_PID_ENABLE,
                 "./log_udpsrv/", "udpsrvlog", "UDPSRV", 10);
#endif
#endif 


    if (fa_network_init()) {
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_SYS_IO, __FILE__, __LINE__);
        return -1;
	}

#ifdef __GNUC__
    fa_sigpipe_init(NULL);
#endif


    strcpy(hostname, "192.168.20.82");
    /*strcpy(hostname, "192.168.20.38");*/
    port = 1984;

	/*create trans unit*/
	trans = fa_create_trans(fa_create_trans_udpsrv, fa_destroy_trans_udpsrv);

	/*try to open trans*/
	if(trans->open(trans,hostname,port)<0){
        printf("open  fail\n");
        return -1;
	}

    recv_len = TRANS_BUF_SIZE;

    while(1) {
        char buf1[128];

        real_recv_len = trans->recv(trans, buf, recv_len);
        if (real_recv_len == -2)
            printf("recv time out, continue try \n");
        else {
            printf("-->want %d, recv %d bytes\n", recv_len, real_recv_len);
#if 1 
            sprintf(buf1, "recv %d bytes\n", real_recv_len);
            trans->send(trans, buf1, strlen(buf1));
#endif
        }

        if(real_recv_len< 0){
            printf("recv fail\n");
            continue;
        }
    }
	
	fa_network_close();


	return 0;


}
