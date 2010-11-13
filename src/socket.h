#ifndef __SOCKET_H
#define __SOCKET_H

#include "common.h"
#include "buffer.h"

#ifdef __WIN32

#include <winsock2.h>

#define ECONNRESET WSAECONNRESET
#define EINPROGRESS WSAEINPROGRESS
#define EALREADY WSAEALREADY
#define ECONNABORTED WSAECONNABORTED
#define ioctl ioctlsocket
#define hstrerror(x) ""
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <netdb.h>
#endif

#define APE_SOCKET_BACKLOG 2048

/* get a ape_socket pointer from fd number */
#define APE_SOCKET(fd, ape) ((ape_socket *)ape->events.fds[fd].data)

typedef enum {
	APE_SOCKET_TCP,
	APE_SOCKET_UDP
} ape_socket_proto;

typedef enum {
	APE_SOCKET_UNKNOWN,
	APE_SOCKET_SERVER,
	APE_SOCKET_CLIENT
} ape_socket_type;

typedef struct {
	buffer data_in;
	buffer data_out;
	int fd;
	ape_socket_type type;
	ape_socket_proto proto;
} ape_socket;

ape_socket *APE_socket_new(ape_socket_proto pt, int from);
int APE_socket_listen(ape_socket *socket, uint16_t port, const char *local_ip, ape_global *ape);

inline int ape_socket_accept(ape_socket *socket, ape_global *ape);
inline int ape_socket_read(ape_socket *socket, ape_global *ape);

#endif
