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


  filename: fa_network.c 
  version : v1.0.0
  time    : 2012/12/5  
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/

#include <stdio.h>
#include <stdlib.h>
#include "fa_network.h"

#ifndef WIN32
#include <strings.h>
#include <string.h>
#include <signal.h>
#else
#include <string.h>
#endif

#include <fcntl.h>
#include <errno.h>

#ifdef WIN32
#pragma comment(lib, "ws2_32.lib") 
#endif


void fa_url_split(char *proto, int proto_size,
                  char *authorization, int authorization_size,
                  char *hostname, int hostname_size,
                  int *port_ptr,
                  char *path, int path_size,
                  const char *url)
{
    const char *p, *ls, *at, *col, *brk;

    if (port_ptr)               *port_ptr = -1;
    if (proto_size > 0)         proto[0] = 0;
    if (authorization_size > 0) authorization[0] = 0;
    if (hostname_size > 0)      hostname[0] = 0;
    if (path_size > 0)          path[0] = 0;

    /* parse protocol */
    if ((p = strchr(url, ':'))) {
        fa_strlcpy(proto, url, FA_MIN(proto_size, p + 1 - url));
        p++; /* skip ':' */
        if (*p == '/') p++;
        if (*p == '/') p++;
    } else {
        /* no protocol means plain filename */
        fa_strlcpy(path, url, path_size);
        return;
    }

    /* separate path from hostname */
    ls = strchr(p, '/');
    if(!ls)
        ls = strchr(p, '?');
    if(ls)
        fa_strlcpy(path, ls, path_size);
    else
        ls = &p[strlen(p)]; // XXX

    /* the rest is hostname, use that to parse auth/port */
    if (ls != p) {
        /* authorization (user[:pass]@hostname) */
        if ((at = strchr(p, '@')) && at < ls) {
            fa_strlcpy(authorization, p,
                       FA_MIN(authorization_size, at + 1 - p));
            p = at + 1; /* skip '@' */
        }

        if (*p == '[' && (brk = strchr(p, ']')) && brk < ls) {
            /* [host]:port */
            fa_strlcpy(hostname, p + 1,
                       FA_MIN(hostname_size, brk - p));
            if (brk[1] == ':' && port_ptr)
                *port_ptr = atoi(brk + 2);
        } else if ((col = strchr(p, ':')) && col < ls) {
            fa_strlcpy(hostname, p,
                       FA_MIN(col + 1 - p, hostname_size));
            if (port_ptr) *port_ptr = atoi(col + 1);
        } else
            fa_strlcpy(hostname, p,
                       FA_MIN(ls + 1 - p, hostname_size));
    }
}



#ifdef WIN32
int fa_inet_aton (const char * str, struct in_addr * add)
{
    unsigned int add1 = 0, add2 = 0, add3 = 0, add4 = 0;

    if (sscanf(str, "%d.%d.%d.%d", &add1, &add2, &add3, &add4) != 4)
        return 0;

    if (!add1 || (add1|add2|add3|add4) > 255) return 0;

    add->s_addr = htonl((add1 << 24) + (add2 << 16) + (add3 << 8) + add4);

    return 1;
}
#endif

#ifdef __GNUC__
int fa_inet_aton (const char * str, struct in_addr * add)
{
    return inet_aton(str, add);
}
#endif /* !HAVE_INET_ATON */


#ifdef WIN32
int fa_getaddrinfo(const char *node, const char *service,
                   const struct addrinfo *hints, struct addrinfo **res)
{
    struct hostent *h = NULL;
    struct addrinfo *ai;
    struct sockaddr_in *sin;


    *res = NULL;
    sin = fa_malloc(sizeof(struct sockaddr_in));
    if (!sin)
        return EAI_FAIL;
    sin->sin_family = AF_INET;

    if (node) {
        if (!fa_inet_aton(node, &sin->sin_addr)) {
            if (hints && (hints->ai_flags & AI_NUMERICHOST)) {
                fa_free(sin);
                return EAI_FAIL;
            }
            h = gethostbyname(node);
            if (!h) {
                fa_free(sin);
                return EAI_FAIL;
            }
            memcpy(&sin->sin_addr, h->h_addr_list[0], sizeof(struct in_addr));
        }
    } else {
        if (hints && (hints->ai_flags & AI_PASSIVE)) {
            sin->sin_addr.s_addr = INADDR_ANY;
        } else
            sin->sin_addr.s_addr = INADDR_LOOPBACK;
    }

    /* Note: getaddrinfo allows service to be a string, which
     * should be looked up using getservbyname. */
    if (service)
        sin->sin_port = htons(atoi(service));

    ai = fa_malloc(sizeof(struct addrinfo));
    if (!ai) {
        fa_free(sin);
        return EAI_FAIL;
    }

    *res = ai;
    ai->ai_family = AF_INET;
    ai->ai_socktype = hints ? hints->ai_socktype : 0;
    switch (ai->ai_socktype) {
    case SOCK_STREAM: ai->ai_protocol = IPPROTO_TCP; break;
    case SOCK_DGRAM:  ai->ai_protocol = IPPROTO_UDP; break;
    default:          ai->ai_protocol = 0;           break;
    }

    ai->ai_addr = (struct sockaddr *)sin;
    ai->ai_addrlen = sizeof(struct sockaddr_in);
    if (hints && (hints->ai_flags & AI_CANONNAME))
        ai->ai_canonname = h ? fa_strdup(h->h_name) : NULL;

    ai->ai_next = NULL;
    return 0;
}

void fa_freeaddrinfo(struct addrinfo *res)
{
    fa_free(res->ai_canonname);
    fa_free(res->ai_addr);
    fa_free(res);
}

int fa_getnameinfo(const struct sockaddr *sa, int salen,
                   char *host, int hostlen,
                   char *serv, int servlen, int flags)
{
    const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;

#ifdef WIN32
    int (WSAAPI *win_getnameinfo)(const struct sockaddr *sa, socklen_t salen,
                                  char *host, DWORD hostlen,
                                  char *serv, DWORD servlen, int flags);
    HMODULE ws2mod = GetModuleHandle("ws2_32.dll");
    win_getnameinfo = GetProcAddress(ws2mod, "getnameinfo");
    if (win_getnameinfo)
        return win_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
#endif

    if (sa->sa_family != AF_INET)
        return EAI_FAMILY;
    if (!host && !serv)
        return EAI_NONAME;

    if (host && hostlen > 0) {
        struct hostent *ent = NULL;
        unsigned int a;
        if (!(flags & NI_NUMERICHOST))
            ent = gethostbyaddr((const char *)&sin->sin_addr,
                                sizeof(sin->sin_addr), AF_INET);

        if (ent) {
            snprintf(host, hostlen, "%s", ent->h_name);
        } else if (flags & NI_NAMERQD) {
            return EAI_NONAME;
        } else {
            a = ntohl(sin->sin_addr.s_addr);
            snprintf(host, hostlen, "%d.%d.%d.%d",
                     ((a >> 24) & 0xff), ((a >> 16) & 0xff),
                     ((a >>  8) & 0xff), ( a        & 0xff));
        }
    }

    if (serv && servlen > 0) {
        struct servent *ent = NULL;
        if (!(flags & NI_NUMERICSERV))
            ent = getservbyport(sin->sin_port, flags & NI_DGRAM ? "udp" : "tcp");

        if (ent) {
            snprintf(serv, servlen, "%s", ent->s_name);
        } else
            snprintf(serv, servlen, "%d", ntohs(sin->sin_port));
    }

    return 0;
}

const char *fa_gai_strerror(int ecode)
{
    switch(ecode) {
    case EAI_FAIL   : return "A non-recoverable error occurred";
    case EAI_FAMILY : return "The address family was not recognized or the address length was invalid for the specified family";
    case EAI_NONAME : return "The name does not resolve for the supplied parameters";
    }

    return "Unknown error";
}
#endif


int fa_socket_nonblock(int socket, int enable)
{
#ifdef WIN32
   return ioctlsocket(socket, FIONBIO, &enable);
#else
   if (enable)
      return fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
   else
      return fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) & ~O_NONBLOCK);
#endif
}




#if 1 //def WIN32
int poll(struct pollfd *fds, nfds_t numfds, int timeout)
{
    fd_set read_set;
    fd_set write_set;
    fd_set exception_set;
    nfds_t i;
    int n;
    int rc;

#ifdef WIN32
    if (numfds >= FD_SETSIZE) {
        errno = EINVAL;
        return -1;
    }
#endif

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_ZERO(&exception_set);

    n = -1;
    for(i = 0; i < numfds; i++) {
        if (fds[i].fd < 0)
            continue;

#ifdef __GNUC__
        if (fds[i].fd >= FD_SETSIZE) {
            errno = EINVAL;
            return -1;
        }
#endif

        if (fds[i].events & POLLIN)  FD_SET(fds[i].fd, &read_set);
        if (fds[i].events & POLLOUT) FD_SET(fds[i].fd, &write_set);
        if (fds[i].events & POLLERR) FD_SET(fds[i].fd, &exception_set);

        if (fds[i].fd > n)
            n = fds[i].fd;
    };

    if (n == -1)
        /* Hey!? Nothing to poll, in fact!!! */
        return 0;

    if (timeout < 0)
        rc = select(n+1, &read_set, &write_set, &exception_set, NULL);
    else {
        struct timeval    tv;

        tv.tv_sec = timeout / 1000;
        tv.tv_usec = 1000 * (timeout % 1000);
        rc = select(n+1, &read_set, &write_set, &exception_set, &tv);
    };

    if (rc < 0)
        return rc;

    for(i = 0; i < (nfds_t) n; i++) {
        fds[i].revents = 0;

        if (FD_ISSET(fds[i].fd, &read_set))      fds[i].revents |= POLLIN;
        if (FD_ISSET(fds[i].fd, &write_set))     fds[i].revents |= POLLOUT;
        if (FD_ISSET(fds[i].fd, &exception_set)) fds[i].revents |= POLLERR;
    };

    return rc;
}
#endif /* HAVE_POLL_H */



//add by luolongzhi to process broken pipe in Linux (in windows, if peer reset will return -10054 error)
#ifdef __GNUC__

static void (* handle_brokenpipe_event_local)() = NULL;
static void handle_brokenpipe();

void fa_sigpipe_init(void (* handle_brokenpipe_event)())
{
    signal(SIGPIPE, handle_brokenpipe);
    handle_brokenpipe_event_local = handle_brokenpipe_event;
}

static void handle_brokenpipe()
{
    printf("boken pipe detected, maybe peer is reset, now will call your own process function to handle this event\n");

    if (handle_brokenpipe_event_local) {
        handle_brokenpipe_event_local();
    }

    /*exit(0);*/
}

#endif

