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


  filename: fa_trans.h 
  version : v1.0.0
  time    : 2012/12/5  
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/

#ifndef FA_TRANS_H
#define FA_TRANS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

typedef struct _fa_trans_t            fa_trans_t;
typedef struct _fa_trans_func_ext_t   fa_trans_func_ext_t;


/*spec_fun used to expand the fa_tran_t implmention function*/
struct _fa_trans_func_ext_t {
	void (*func_ext1_seek)();
};

/*
    NOTE: open function real instatnce , current support interface
          int tcp_open(fa_trans_t *trans, char *hostname, int port);                  // destination hostname and port 
          int udp_open(fa_trans_t *trans, char *hostname, int port);                  // destination hostname and port
          int udp_open_bind(fa_trans_t *trans, char *hostname, int port,
                                               char *localname,int localport);        // destination hostname and port, localhostname and port(for bind)
          int udpsrv_open(fa_trans_t *trans, char *hostname, int port);               // localhost name and port(for bind)
*/
struct _fa_trans_t {
	int (*open) (fa_trans_t *trans, ...);
	int (*send) (fa_trans_t *trans, char *data , int data_len);
	int (*recv) (fa_trans_t *trans, char *data , int data_len);
	int (*close)(fa_trans_t *trans);

	char *trans_name;
	void *priv_data;	            //the addr of the specific trans protocol context
	fa_trans_func_ext_t *func_ext;	//reserved for the function extension-> point to the extension func union

    int (*destroy_trans)(fa_trans_t *trans);
};


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
