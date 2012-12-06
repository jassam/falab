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


  filename: fa_tcpsrv.h 
  version : v1.0.0
  time    : 2012/12/5  
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/

#ifndef FA_TCP_SERVER_H
#define FA_TCP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif


#define	CLIENT_MAX_NUM	2000

unsigned long fa_tcpsrv_init(int (* handle_new_connection_callback)(int socket_fd), 
                             int (* handle_connection_read_callback)(int socket_fd));

void fa_tcpsrv_uninit(unsigned long handle);

int fa_tcpsrv_start(unsigned long handle, const char *hostname, const int port);

int fa_tcpsrv_stop(unsigned long handle);

int fa_recvfrom_client(int fd, char *buf, int size);

int fa_sendto_client(int fd, char *buf, int size);

void fa_tcpsrv_getstatus(unsigned long handle, int *client_nb, int *is_running, int *heart_hop);


#ifdef __cplusplus
}
#endif



#endif
