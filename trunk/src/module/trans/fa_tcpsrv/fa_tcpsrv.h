#ifndef FA_TCP_SERVER_H
#define FA_TCP_SERVER_H

int fa_tcpsrv_init(int (* handle_new_connection_callback)(int socket_fd), 
                   int (* handle_connection_read_callback)(int socket_fd));
int fa_tcpsrv_uninit();

int fa_tcpsrv_start(const char *hostname, const int port);
int fa_tcpsrv_stop();

#endif
