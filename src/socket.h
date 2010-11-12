#ifndef __SOCKET_H
#define __SOCKET_H

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

typedef enum {
	APE_SOCKET_TCP,
	APE_SOCKET_UDP
} ape_socket_proto;

typedef enum {
	APE_SOCKET_SERVER,
	APE_SOCKET_CLIENT
} ape_socket_type;

#endif
