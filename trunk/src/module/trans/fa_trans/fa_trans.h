#ifndef FA_TRANS_H
#define FA_TRANS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

typedef struct _fa_trans            fa_trans_t;
typedef struct _fa_trans_func_ext   fa_trans_func_ext_t;


/*spec_fun used to expand the fa_tran_t implmention function*/
struct _fa_trans_func_ext {
	void (*func_ext1_seek)();
};


struct _fa_trans {
	int (*open) (fa_trans_t *trans, ...);
	int (*send) (fa_trans_t *trans, char *data , int data_len);
	int (*recv) (fa_trans_t *trans, char *data , int data_len);
	int (*close)(fa_trans_t *trans);

	char *trans_name;
	void *priv_data;	            //the addr of the specific trans protocol context
	fa_trans_func_ext_t *func_ext;	//reserved for the function extension-> point to the extension func union

    int (*destroy_trans)(fa_trans_t *trans);
};


int fa_trans_init();
int fa_trans_uninit();

/*this is the common create, the child_arg specified by the specific trans_name
  call the "tcp"or"udp" private global function to implemention*/
fa_trans_t * fa_create_trans(int (* create_trans_callback)(fa_trans_t *trans),
                             int (* destroy_trans_callback)(fa_trans_t *trans));

int          fa_destroy_trans(fa_trans_t *trans);



//sys error 
#define FA_ERR_SYS_INVALIDDATA	    "Invalid data found when processing input"
#define FA_ERR_SYS_IO               "I/O error"
#define FA_ERR_SYS_NOENT            "No such file or directory"
#define FA_ERR_SYS_NOFMT	        "Unknown format" 
#define FA_ERR_SYS_NOMEM            "Not enough memory"		 
#define FA_ERR_SYS_NOTSUPP	        "Operation not supported"	 
#define FA_ERR_SYS_NUMEXPECTED	    "Number syntax expected in filename" 
#define FA_ERR_SYS_EBADF            "bad file, no file or write attempt to a read-only"
#define FA_ERR_SYS_UNKNOWN	        "Unknown sys error" 

//network 
#define FA_ERR_NETWORK_OPEN         "network ope fail"
#define FA_ERR_NETWORK_SEND         "network send fail"
#define FA_ERR_NETWORK_RECV         "network recv fail"
#define FA_ERR_NETWORK_PORTNO       "network port set error"
#define FA_ERR_NETWORK_CONNECT      "network connect fail"


#ifdef __cplusplus
}
#endif


#endif
