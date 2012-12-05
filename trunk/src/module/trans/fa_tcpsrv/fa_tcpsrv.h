#ifndef FA_TCP_SERVER_H
#define FA_TCP_SERVER_H

#define	CLIENT_MAX_NUM	2000

unsigned long fa_tcpsrv_init(int (* handle_new_connection_callback)(int socket_fd), 
                             int (* handle_connection_read_callback)(int socket_fd));

void fa_tcpsrv_uninit(unsigned long handle);

int fa_tcpsrv_start(unsigned long handle, const char *hostname, const int port);

int fa_tcpsrv_stop(unsigned long handle);

int fa_recvfrom_client(int fd, char *buf, int size);

int fa_sendto_client(int fd, char *buf, int size);

void fa_tcpsrv_getstatus(unsigned long handle, int *client_nb, int *is_running, int *heart_hop);

#endif
