#ifndef __APE_SOCKET_H
#define __APE_SOCKET_H

#include "common.h"
#include "buffer.h"
#include "ape_pool.h"

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

/* get a ape_socket pointer from event returns */
#define APE_SOCKET(attach) ((ape_socket *)attach)

#ifdef TCP_CORK
	#define PACK_TCP(fd) \
	do { \
		int __state = 1; \
		setsockopt(fd, IPPROTO_TCP, TCP_CORK, &__state, sizeof(__state)); \
	} while(0)

	#define FLUSH_TCP(fd) \
	do { \
		int __state = 0; \
		setsockopt(fd, IPPROTO_TCP, TCP_CORK, &__state, sizeof(__state)); \
	} while(0)
#else
	#define PACK_TCP(fd)
	#define FLUSH_TCP(fd)
#endif


#define APE_SOCKET_UNSET_BITS(bits, mask) bits = bits & mask & 0x00 /* sry */
#define APE_SOCKET_SET_BITS(bits, mask) bits = bits & mask
#define APE_SOCKET_HAS_BITS(bits, mask) (bits & (0x00 & mask))

#define APE_SOCKET_WOULD_BLOCK	0x00FFFFFF | (0x01 << 24)

#define APE_SOCKET_PT_TCP 	0xFF00FFFF | (0x01 << 16)
#define APE_SOCKET_PT_UDP 	0xFF00FFFF | (0x02 << 16)

#define APE_SOCKET_TP_UNKNOWN 	0xFFFF00FF | (0x01 << 8)
#define APE_SOCKET_TP_SERVER 	0xFFFF00FF | (0x02 << 8)
#define APE_SOCKET_TP_CLIENT 	0xFFFF00FF | (0x04 << 8)

#define APE_SOCKET_ST_ONLINE	0xFFFFFF00 | (0x01)
#define APE_SOCKET_ST_PROGRESS	0xFFFFFF00 | (0x02)
#define APE_SOCKET_ST_PENDING	0xFFFFFF00 | (0x04)
#define APE_SOCKET_ST_OFFLINE	0xFFFFFF00 | (0x08)

typedef struct _ape_socket ape_socket;


typedef struct {
	void (*on_read)		(ape_socket *, ape_global *);
	void (*on_disconnect)	(ape_socket *, ape_global *);
	void (*on_connect)	(ape_socket *, ape_global *);
} ape_socket_callbacks;


/* Jobs pool */
/* (1 << 0) is reserved */
#define APE_SOCKET_JOB_WRITEV (1 << 1)
#define APE_SOCKET_JOB_SENDFILE (1 << 2)
#define APE_SOCKET_JOB_SHUTDOWN (1 << 3)
#define APE_SOCKET_JOB_ACTIVE (1 << 4)

typedef ape_pool_t ape_socket_jobs_t;


struct _ape_socket {
	ape_fds s;
	
	buffer data_in;
	buffer data_out;
	
	struct {
		ape_socket_jobs_t *list;
		ape_socket_jobs_t *last;
		/*
		TODO: add last active
		*/
	} jobs;
	
	struct {
		int fd;
		int offset;
	} file_out;
	
	void *ctx; 	/* public pointer */
	
	ape_socket_callbacks 	callbacks;
	
	uint32_t		flags;
	uint16_t 		remote_port;
};

struct _ape_socket_packet {
	char *ptr;
	size_t len;
	size_t offset;
} typedef ape_socket_packet_t;

ape_socket *APE_socket_new(uint32_t pt, int from);

int APE_socket_listen(ape_socket *socket, uint16_t port, const char *local_ip, ape_global *ape);
int APE_socket_connect(ape_socket *socket, uint16_t port, const char *remote_ip_host, ape_global *ape);
int APE_socket_write(ape_socket *socket, char *data, size_t len);
int APE_socket_destroy(ape_socket *socket, ape_global *ape);

inline int ape_socket_accept(ape_socket *socket, ape_global *ape);
inline int ape_socket_read(ape_socket *socket, ape_global *ape);
int ape_socket_write_file(ape_socket *socket, const char *file, ape_global *ape);

#endif
