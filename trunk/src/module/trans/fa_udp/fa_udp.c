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


  filename: fa_udp.c 
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

#include "fa_udp.h"
#include "fa_network.h"
#include "fa_malloc.h"
#include "fa_print.h"


typedef struct _udp_context {
    int fd;
	struct addrinfo *ai;
	struct addrinfo *ai1;
}udp_context_t;


static int udp_open(fa_trans_t *trans, char *hostname, int port)
{
	
//	va_list vargs;
//	int port;
//	char *hostname;

	/*struct addrinfo hints, *ai, *cur_ai;*/
    struct addrinfo hints, *cur_ai;

    int fd_max;
	udp_context_t *s = NULL;
	fd_set wfds, efds;
    struct timeval tv;
    socklen_t optlen;
	int ret;
    char portstr[10];
	int fd = -1;
    int tmp;

//	va_start(vargs,trans);
//	hostname = va_arg(vargs,char *);
//	port = va_arg(vargs,int);

	if (port <= 0 || port >= 65536) {
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_NETWORK_PORTNO, __FILE__, __LINE__);
		return -1;	//port is not correct
	}

    s = fa_malloc(sizeof(udp_context_t));
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

    fa_socket_nonblock(fd, 1);

    trans->priv_data = s;
    s->fd = fd;

    return 0;

fail:
    FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_SYS_IO, __FILE__, __LINE__);
    ret = -1;
    if (fd >= 0)
        closesocket(fd);
    if (s) {
        freeaddrinfo(s->ai);
        fa_free(s);
    }

    return ret;
}


static int udp_open_bind(fa_trans_t *trans, char *hostname, int port,
                                            char *localname,int localport)
{
	
//	va_list vargs;
//	int port;
//	char *hostname;

	/*struct addrinfo hints, *ai, *cur_ai;*/
    struct addrinfo hints, *cur_ai;

    int fd_max;
	udp_context_t *s = NULL;
	fd_set wfds, efds;
    struct timeval tv;
    socklen_t optlen;
	int ret;
    char portstr[10];
	int fd = -1;
    int tmp;

//	va_start(vargs,trans);
//	hostname = va_arg(vargs,char *);
//	port = va_arg(vargs,int);

	if (port <= 0 || port >= 65536) {
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_NETWORK_PORTNO, __FILE__, __LINE__);
		return -1;	//port is not correct
	}

    s = fa_malloc(sizeof(udp_context_t));
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

#if 1 
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    snprintf(portstr, sizeof(portstr), "%d", localport);
    ret = getaddrinfo(localname, portstr, &hints, &(s->ai1));	//cvt the hostname to the address(support ipv6)
    if (ret) {
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_SYS_IO, __FILE__, __LINE__);
        goto fail;
    }
    cur_ai = s->ai1;
#endif

    fd = socket(cur_ai->ai_family, cur_ai->ai_socktype, cur_ai->ai_protocol);
    if (fd < 0)
        goto fail;
#if 1 
    tmp = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

    if (bind (fd, cur_ai->ai_addr, cur_ai->ai_addrlen) < 0) {
        char bindmsg[32];
        /*snprintf(bindmsg, sizeof(bindmsg), "bind(port %d)", ntohs(cur_ai->ai_addr->sin_port));*/
        snprintf(bindmsg, sizeof(bindmsg), "bind fail");
//        perror (bindmsg);
        FA_PRINT("FAIL: %s , [err at: %s-%d]\n", bindmsg, __FILE__, __LINE__);
        closesocket(fd);
        goto fail;
    }
#endif

    fa_socket_nonblock(fd, 1);

    trans->priv_data = s;
    s->fd = fd;

    return 0;

fail:
    FA_PRINT("FAIL: %s , [err at: %s-%d]\n", FA_ERR_SYS_IO, __FILE__, __LINE__);
    ret = -1;
    if (fd >= 0)
        closesocket(fd);
    if (s) {
        freeaddrinfo(s->ai);
        fa_free(s);
    }

    return ret;
}


static int udp_send(fa_trans_t *trans, char *buf , int size)
{
    udp_context_t *s = trans->priv_data;
    int ret, size1, fd_max, len;
    fd_set wfds;
    struct timeval tv;

    size1 = size;
    while (size > 0) {
        fd_max = s->fd;
        FD_ZERO(&wfds);
        FD_SET(s->fd, &wfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        ret = select(fd_max + 1, NULL, &wfds, NULL, &tv);
        if (ret > 0 && FD_ISSET(s->fd, &wfds)) {
            len = sendto(s->fd, buf, size, 0, s->ai->ai_addr, s->ai->ai_addrlen);
            if (len < 0) {
                if (fa_neterrno() != FA_NETERROR(EINTR) &&
                    fa_neterrno() != FA_NETERROR(EAGAIN))
                    return fa_neterrno();
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

/*NOTE: if return -2 means timeout*/
static int udp_recv(fa_trans_t *trans, char *buf , int size)
{
    udp_context_t*s = trans->priv_data;
    int len, fd_max, ret;
    fd_set rfds;
    struct timeval tv;
    int max_check_cnt = 10;

    for (;;) {
        max_check_cnt--;

        if (max_check_cnt < 0)
            return -2;

        fd_max = s->fd;
        FD_ZERO(&rfds);
        FD_SET(s->fd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;	//100ms check once
        ret = select(fd_max + 1, &rfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(s->fd, &rfds)) {
            len = recvfrom(s->fd, buf, size, 0, s->ai->ai_addr, &s->ai->ai_addrlen);
            if (len < 0) {
                if (fa_neterrno() != FA_NETERROR(EINTR) &&
                    fa_neterrno() != FA_NETERROR(EAGAIN))
                    return fa_neterrno();
            } else return len;
        } else if (ret < 0) {
            if (fa_neterrno() == FA_NETERROR(EINTR))
                continue;
            return -1;
        }
    }
}

static int udp_close(fa_trans_t *trans)
{
    udp_context_t *s = trans->priv_data;
    closesocket(s->fd);
    fa_free(s);
	return 0;
}

int fa_create_trans_udp(fa_trans_t *trans)
{
	trans->open = udp_open;
	trans->send = udp_send;
	trans->recv = udp_recv;
	trans->close = udp_close;
	trans->trans_name = strdup("udp");
	trans->priv_data = NULL;

	return 0;
}


int fa_create_trans_udpbind(fa_trans_t *trans)
{
	trans->open = udp_open_bind;
	trans->send = udp_send;
	trans->recv = udp_recv;
	trans->close = udp_close;
	trans->trans_name = strdup("udp");
	trans->priv_data = NULL;

	return 0;
}



int fa_destroy_trans_udp(fa_trans_t *trans)
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


