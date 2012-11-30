#ifndef FA_NETWORK_H
#define FA_NETWORK_H

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#ifdef __GNUC__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#ifdef __GNUC__
#include <arpa/inet.h>
#endif

#include "fa_malloc.h"


#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#define fa_neterrno() (-WSAGetLastError())
#define FA_NETERROR(err) (-WSA##err)
#define WSAEAGAIN WSAEWOULDBLOCK

typedef int socklen_t;

#endif

#ifdef __GNUC__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define fa_neterrno() (errno)
#define FA_NETERROR(err) (err)

#define closesocket close

#endif



int fa_socket_nonblock(int socket, int enable);

static inline int fa_network_init(void)
{
#ifdef WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1,1), &wsaData))
        return -1;
#endif

    return 0;
}

static inline void fa_network_close(void)
{
#ifdef WIN32
    WSACleanup();
#endif
}


void fa_url_split(char *proto, int proto_size,
                  char *authorization, int authorization_size,
                  char *hostname, int hostname_size,
                  int *port_ptr,
                  char *path, int path_size,
                  const char *url);


int fa_inet_aton (const char * str, struct in_addr * add);

#ifdef WIN32
struct addrinfo {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    int ai_addrlen;
    struct sockaddr_in *ai_addr;
    char *ai_canonname;
    struct addrinfo *ai_next;
};
typedef int socklen_t;

#endif

/* getaddrinfo constants */
#ifndef EAI_FAIL
#define EAI_FAIL 4
#endif

#ifndef EAI_FAMILY
#define EAI_FAMILY 5
#endif

#ifndef EAI_NONAME
#define EAI_NONAME 8
#endif

#ifndef AI_PASSIVE
#define AI_PASSIVE 1
#endif

#ifndef AI_CANONNAME
#define AI_CANONNAME 2
#endif

#ifndef AI_NUMERICHOST
#define AI_NUMERICHOST 4
#endif

#ifndef NI_NOFQDN
#define NI_NOFQDN 1
#endif

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 2
#endif

#ifndef NI_NAMERQD
#define NI_NAMERQD 4
#endif

#ifndef NI_NUMERICSERV
#define NI_NUMERICSERV 8
#endif

#ifndef NI_DGRAM
#define NI_DGRAM 16
#endif


#ifdef WIN32
int fa_getaddrinfo(const char *node, const char *service,
                   const struct addrinfo *hints, struct addrinfo **res);
void fa_freeaddrinfo(struct addrinfo *res);
int fa_getnameinfo(const struct sockaddr *sa, int salen,
                   char *host, int hostlen,
                   char *serv, int servlen, int flags);
const char *fa_gai_strerror(int ecode);
#define getaddrinfo fa_getaddrinfo
#define freeaddrinfo fa_freeaddrinfo
#define getnameinfo fa_getnameinfo
#define gai_strerror fa_gai_strerror
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN INET_ADDRSTRLEN
#endif


#ifndef IN_MULTICAST
#define IN_MULTICAST(a) ((((unsigned int)(a)) & 0xf0000000) == 0xe0000000)
#endif
#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(a) (((unsigned char *) (a))[0] == 0xff)
#endif

static inline int fa_is_multicast_address(struct sockaddr *addr)
{
    if (addr->sa_family == AF_INET) {
        return IN_MULTICAST(ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr));
    }
#if 1 //HAVE_STRUCT_SOCKADDR_IN6
    if (addr->sa_family == AF_INET6) {
        return IN6_IS_ADDR_MULTICAST(&((struct sockaddr_in6 *)addr)->sin6_addr);
    }
#endif

    return 0;
}


#if 1//def WIN32
typedef unsigned long nfds_t;

struct pollfd {
    int fd;
    short events;  /* events to look for */
    short revents; /* events that occurred */
};

/* events & revents */
#define POLLIN     0x0001  /* any readable data available */
#define POLLOUT    0x0002  /* file descriptor is writeable */
#define POLLRDNORM POLLIN
#define POLLWRNORM POLLOUT
#define POLLRDBAND 0x0008  /* priority readable data */
#define POLLWRBAND 0x0010  /* priority data can be written */
#define POLLPRI    0x0020  /* high priority readable data */

/* revents only */
#define POLLERR    0x0004  /* errors pending */
#define POLLHUP    0x0080  /* disconnected */
#define POLLNVAL   0x1000  /* invalid file descriptor */


int poll(struct pollfd *fds, nfds_t numfds, int timeout);
#endif /* HAVE_POLL_H */


#endif
