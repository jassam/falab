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


  filename: fa_udpsrv.c 
  version : v1.0.0
  time    : 2012/12/5  
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <string.h>
#endif

#ifdef __GNUC__
#include <strings.h>
#endif

#include <errno.h>

#include "fa_udpsrv.h"
#include "fa_network.h"
#include "fa_malloc.h"
#include "fa_print.h"

/*#define UDP_USE_BLOCK*/

typedef struct _udpsrv_context {
    int fd;
	struct addrinfo *ai;
    struct sockaddr_in src_addr;
}udpsrv_context_t;


static int udpsrv_open(fa_trans_t *trans, char *hostname, int port)
{
	
    struct addrinfo hints, *cur_ai;
	udpsrv_context_t *s = NULL;
	int ret;
    char portstr[10];
	int fd = -1;
    int tmp;

	if (port <= 0 || port >= 65536) {
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_NETWORK_PORTNO, __FILE__, __LINE__);
		return -1;	//port is not correct
	}

    s = fa_malloc(sizeof(udpsrv_context_t));
    if (!s) {
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_SYS_NOMEM, __FILE__, __LINE__);
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    snprintf(portstr, sizeof(portstr), "%d", port);
    ret = getaddrinfo(hostname, portstr, &hints, &(s->ai));	//cvt the hostname to the address(support ipv6)
    if (ret) {
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_SYS_IO, __FILE__, __LINE__);
        goto fail;
    }
    cur_ai = s->ai;

    fd = socket(cur_ai->ai_family, cur_ai->ai_socktype, cur_ai->ai_protocol);
    if (fd < 0)
        goto fail;

    tmp = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

    if (bind (fd, cur_ai->ai_addr, cur_ai->ai_addrlen) < 0) {
        char bindmsg[32];
        /*snprintf(bindmsg, sizeof(bindmsg), "bind(port %d)", ntohs(cur_ai->ai_addr->sin_port));*/
        snprintf(bindmsg, sizeof(bindmsg), "bind fail");
//        perror (bindmsg);
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", bindmsg, __FILE__, __LINE__);
        closesocket(fd);
        return -1;
    }

#ifdef UDP_USE_BLOCK
    fa_socket_nonblock(fd, 0);
#else 
    fa_socket_nonblock(fd, 1);
#endif

    s->fd = fd;

    trans->priv_data = s;

    return 0;

fail:
    FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_SYS_IO, __FILE__, __LINE__);
    if (fd >= 0)
        closesocket(fd);
    if (s) {
        if (s->ai)
            freeaddrinfo(s->ai);
        fa_free(s);
    }

    return -1;
}

static int udpsrv_send(fa_trans_t *trans, char *buf , int size)
{
    udpsrv_context_t *s = trans->priv_data;
    int ret, size1, fd_max, len;
    fd_set wfds;
    struct timeval tv;
    int addr_len;

    size1 = size;
    addr_len = sizeof(struct sockaddr);
    while (size > 0) {
        fd_max = s->fd;
        FD_ZERO(&wfds);
        FD_SET(s->fd, &wfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        ret = select(fd_max + 1, NULL, &wfds, NULL, &tv);
        if (ret > 0 && FD_ISSET(s->fd, &wfds)) {
            printf("send to %s:%d\n", inet_ntoa(s->src_addr.sin_addr), ntohs(s->src_addr.sin_port));
            len = sendto(s->fd, buf, size, 0, (struct sockaddr*)&s->src_addr,addr_len);
            if (len < 0) {
                if (fa_neterrno() != FA_NETERROR(EINTR) &&
                    fa_neterrno() != FA_NETERROR(EAGAIN))
                    return -1;//im_neterrno();
                continue;
            }
            size -= len;
            buf += len;
        } else if (ret < 0) {
            if (fa_neterrno() == FA_NETERROR(EINTR))
                continue;
            return -1;
        }
    }
    return size1 - size;
}

static int udpsrv_recv(fa_trans_t *trans, char *buf , int size)
{
    udpsrv_context_t *s = trans->priv_data;
    int len, fd_max, ret;
    fd_set rfds;
    struct timeval tv;
    int addr_len;
    int max_check_cnt=10;

    len = 0;
#ifndef UDP_USE_BLOCK
    addr_len = sizeof(struct sockaddr);
    for(;;) {
        max_check_cnt--;

        if (max_check_cnt < 0)
            return -2;  // -2 means timeout

        fd_max = s->fd;
        FD_ZERO(&rfds);
        FD_SET(s->fd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;	//100ms check once
        ret = select(fd_max + 1, &rfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(s->fd, &rfds)) {
            len = recvfrom(s->fd, buf, size, 0, (struct sockaddr*)&s->src_addr,&addr_len);
            printf("recv from %s:%d\n", inet_ntoa(s->src_addr.sin_addr), ntohs(s->src_addr.sin_port));
            if (len < 0) {
                if (fa_neterrno() != FA_NETERROR(EINTR) &&
                    fa_neterrno() != FA_NETERROR(EAGAIN))
                    return fa_neterrno();
                continue;
             } else  {
                 return len;
             }
        } else if (ret < 0) {
            if (fa_neterrno() == FA_NETERROR(EINTR))
                continue;
            return -1;
        }
    }
    
    return len;

#else 
    /*block recv mode*/

    addr_len = sizeof(struct sockaddr);
    if ((len = recvfrom(s->fd,buf,size,0,
                       (struct sockaddr*)&s->src_addr,&addr_len)) == -1) {
        perror("recvfrom fail\n");
        return -1;
    }

    buf[len]='\0';
    printf("recv %d\n", len);
    printf("%s\n", inet_ntoa(s->src_addr.sin_addr));

    return len;


#endif


}

static int udpsrv_close(fa_trans_t *trans)
{
    udpsrv_context_t *s = trans->priv_data;
    closesocket(s->fd);
    fa_free(s);
	return 0;
}

int fa_create_trans_udpsrv(fa_trans_t *trans)
{
	trans->open = udpsrv_open;
	trans->send = udpsrv_send;
	trans->recv = udpsrv_recv;
	trans->close = udpsrv_close;
	trans->trans_name = strdup("udpsrv");
	trans->priv_data = NULL;

	return 0;
}

int fa_destroy_trans_udpsrv(fa_trans_t *trans)
{
    if (trans->trans_name)
        fa_free(trans->trans_name);

	if (trans->priv_data)
		fa_free(trans->priv_data);

	trans->open = NULL;
	trans->send = NULL;
	trans->recv = NULL;
	trans->close = NULL;

	return 0;

}


