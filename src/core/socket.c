/*
  Copyright (C) 2010  Anthony Catel <a.catel@weelya.com>

  This file is part of APE Server.
  APE is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  APE is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with APE ; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

/* sock.c */

#include "socket.h"
#include "dns.h"

#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/sendfile.h>
#include <limits.h>

/* 
	Use only one syscall (ioctl) if FIONBIO is defined
	It behaves the same for socket file descriptor to use either ioctl(...FIONBIO...) or fcntl(...O_NONBLOCK...)
*/
#ifdef FIONBIO
static inline int setnonblocking(int fd)
{	
    int  ret = 1;

    return ioctl(fd, FIONBIO, &ret);	
}
#else
#define setnonblocking(fd) fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)
#endif

static ape_socket_jobs_t *ape_socket_new_jobs_queue(size_t n);
static ape_socket_jobs_t *ape_socket_job_get_slot(ape_socket *socket, int type);
static int ape_socket_queue_data(ape_socket *socket, char *data, size_t len, int offset);
static ape_pool_list_t *ape_socket_new_packet_queue(size_t n);
static void ape_init_job_list(ape_pool_list_t *list, size_t n);

ape_socket *APE_socket_new(uint8_t pt, int from)
{
	int sock = from, proto = SOCK_STREAM;
	
	ape_socket *ret = NULL;
	
#ifdef __WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 2, 2 );

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		return NULL;
	}
	
	/* TODO WSAClean et al */
#endif

	proto = (pt == APE_SOCKET_PT_UDP ? SOCK_DGRAM : SOCK_STREAM);
	
	if ((sock == 0 && 
		(sock = socket(AF_INET /* TODO AF_INET6 */, proto, 0)) == -1) || 
		setnonblocking(sock) == -1) {
		return NULL;
	}

	ret 			= malloc(sizeof(*ret));
	ret->s.fd 		= sock;
	ret->s.type		= APE_SOCKET;
	ret->states.flags 	= 0;
	ret->states.type 	= APE_SOCKET_TP_UNKNOWN;
	ret->states.state 	= APE_SOCKET_ST_PENDING;
	ret->states.proto 	= pt;

	
	ret->callbacks.on_read 		= NULL;
	ret->callbacks.on_disconnect 	= NULL;
	ret->callbacks.on_connect	= NULL;
	
	ret->remote_port = 0;

	buffer_init(&ret->data_in);
	buffer_init(&ret->data_out);

	ape_init_job_list(&ret->jobs, 2);
	
	ret->file_out.fd 	= 0;
	ret->file_out.offset 	= 0;
	
	return ret;
}

int APE_socket_listen(ape_socket *socket, uint16_t port, 
		const char *local_ip, ape_global *ape)
{
	struct sockaddr_in addr;
	int reuse_addr = 1;
	
	if (port == 0 || port > 65535) {
		return -1;
	}	
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(local_ip);
	memset(&(addr.sin_zero), '\0', 8);
	
	setsockopt(socket->s.fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int));

	if (bind(socket->s.fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1 ||
		(socket->states.proto == APE_SOCKET_PT_TCP && /* only listen for STREAM socket */
		listen(socket->s.fd, APE_SOCKET_BACKLOG) == -1)) {
		
		close(socket->s.fd);
		
		return -1;
	}
	
	socket->states.type = APE_SOCKET_TP_SERVER;
	socket->states.state = APE_SOCKET_ST_ONLINE;
	
	events_add(socket->s.fd, socket, EVENT_READ|EVENT_WRITE, ape);

	return 0;
	
}

static int ape_socket_connect_ready_to_connect(const char *remote_ip, 
		void *arg, int status, ape_global *ape)
{
	ape_socket *socket = arg;
	struct sockaddr_in addr;
	
	if (status != ARES_SUCCESS) {
		APE_socket_destroy(socket, ape);
		return -1;
	}
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(socket->remote_port);
	addr.sin_addr.s_addr = inet_addr(remote_ip);
	memset(&(addr.sin_zero), '\0', 8);	
	
	if (connect(socket->s.fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == 0 || 
		errno != EINPROGRESS) {
		
		APE_socket_destroy(socket, ape);
		return -1;
	}
	
	socket->states.type = APE_SOCKET_TP_CLIENT;
	socket->states.state = APE_SOCKET_ST_PROGRESS;
	
	events_add(socket->s.fd, socket, EVENT_READ|EVENT_WRITE, ape);
	
	return 0;
	
}

int APE_socket_connect(ape_socket *socket, uint16_t port, 
		const char *remote_ip_host, ape_global *ape)
{
	if (port == 0 || port > 65535) {
		APE_socket_destroy(socket, ape);
		return -1;
	}
	
	socket->remote_port = port;
	ape_gethostbyname(remote_ip_host, ape_socket_connect_ready_to_connect, socket, ape);
	
	return 0;
}

void APE_socket_shutdown(ape_socket *socket)
{
	if (socket->states.state != APE_SOCKET_ST_ONLINE) {
		return;
	}
	if (socket->states.flags & APE_SOCKET_WOULD_BLOCK) {
		ape_socket_job_get_slot(socket, APE_SOCKET_JOB_SHUTDOWN);
		return;
	}
	printf("Direct shutdown\n");
	shutdown(socket->s.fd, 2);
}

void APE_sendfile(ape_socket *socket, const char *file)
{
	int fd, nwrite = 0;
	off_t offset = 0;
	ape_socket_jobs_t *job;
	
	if ((fd = open(file, O_RDONLY)) == -1) {
		printf("Unable to open file\n");
		return;
	}
	PACK_TCP(socket->s.fd);
	while (nwrite != -1) {
		nwrite = sendfile(socket->s.fd, fd, NULL, 4096);
		if (nwrite == 0) {
			FLUSH_TCP(socket->s.fd);
			return;
		}
	}
	
	if (errno == EAGAIN) {
		socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
		job 		= ape_socket_job_get_slot(socket, APE_SOCKET_JOB_SENDFILE);
		job->ptr.fd 	= fd;
	}
	
	FLUSH_TCP(socket->s.fd);
	
}

int APE_socket_write(ape_socket *socket, char *data, size_t len)
{
	ssize_t t_bytes = 0, r_bytes = len, n = 0;

	if (socket->states.state != APE_SOCKET_ST_ONLINE || 
			len == 0) {
		return -1;
	}

	if (socket->states.flags & APE_SOCKET_WOULD_BLOCK /* || something in the queue */) {
		ape_socket_queue_data(socket, data, len, 0);
		printf("Would block\n");
		return -1;
	}
	
	while (t_bytes < len) {
		if ((n = write(socket->s.fd, data + t_bytes, r_bytes)) < 0) {
			if (errno == EAGAIN && r_bytes != 0) {
				socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
				ape_socket_queue_data(socket, data, len, t_bytes);
				printf("Write not finished %d (done : %d)\n", r_bytes, t_bytes);
				return r_bytes;
			} else {
				return -1;
			}
		}
		
		t_bytes += n;
		r_bytes -= n;		
	}
	/* TODO : free data ? */
	//printf("Success %d\n", t_bytes);
	return 0;
}

int APE_socket_destroy(ape_socket *socket, ape_global *ape)
{
	close(socket->s.fd);
	
	buffer_delete(&socket->data_in);
	buffer_delete(&socket->data_out);
	
	socket->states.state = APE_SOCKET_ST_OFFLINE;
	
	ape_destroy_pool(socket->jobs.head);
	
	free(socket);
}

int ape_socket_do_jobs(ape_socket *socket)
{

#if defined(IOV_MAX)
	const size_t max_chunks = IOV_MAX;
#elif defined(MAX_IOVEC)
	const size_t max_chunks = MAX_IOVEC;
#elif defined(UIO_MAXIOV)
	const size_t max_chunks = UIO_MAXIOV;
#elif (defined(__FreeBSD__) && __FreeBSD_version < 500000) || defined(__DragonFly__) || defined(__APPLE__) 
	const size_t max_chunks = 1024;
#elif defined(_SC_IOV_MAX)
	const size_t max_chunks = sysconf(_SC_IOV_MAX);
#else
#error "Cannot get the _SC_IOV_MAX value"
#endif	
	ape_socket_jobs_t *job;
	struct iovec chunks[max_chunks];
	
	job = socket->jobs.head;
	
	//printf("Jobs to do\n");
	
	while(job != NULL && job->flags & APE_SOCKET_JOB_ACTIVE) {
		switch(job->flags & ~(APE_POOL_ALL_FLAGS | APE_SOCKET_JOB_ACTIVE)) {
		case APE_SOCKET_JOB_WRITEV:
		{
			int i, y;
			ssize_t n, total_len = 0;
			ape_pool_list_t *plist = (ape_pool_list_t *)job->ptr.data;
			ape_socket_packet_t *packet = (ape_socket_packet_t *)plist->head;

			for (i = 0; packet != NULL && i < max_chunks; i++) {
				if (packet->pool.ptr.data == NULL) {
					break;
				}
				chunks[i].iov_base = packet->pool.ptr.data + packet->offset;
				chunks[i].iov_len  = packet->len - packet->offset;
			
				//total_len += chunks[i].iov_len;
			
				packet = (ape_socket_packet_t *)packet->pool.next;

			}
		
			n = writev(socket->s.fd, chunks, i);
			if (n == -1) {
				job = job->next;
				continue;
			}

			packet = (ape_socket_packet_t *)plist->head;
		
			while (packet != NULL && packet->pool.ptr.data != NULL) {
				n -= packet->len - packet->offset;
			
				/* packet not finished */
				if (n < 0) {
					packet->offset = packet->len + n;
					break;
				}
			
				free(packet->pool.ptr.data);
				packet->pool.ptr.data = NULL;
			
				packet = (ape_socket_packet_t *)ape_pool_head_to_queue(plist);
			
				if (n == 0) {
					break;
				}
			}
			if (packet->pool.ptr.data == NULL) {
				/* TODO: remove jobs */
				//printf("No more to write\n");
			} else {
				socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
				//printf("Job not finished\n");
				return 0;
			}	
		}
		break;
		case APE_SOCKET_JOB_SENDFILE:
		{
			int nwrite;
			
			while (nwrite > 0) {
				nwrite = sendfile(socket->s.fd, job->ptr.fd, NULL, 4096);
			}
			if (nwrite == -1 && errno == EAGAIN) {
				socket->states.flags |= APE_SOCKET_WOULD_BLOCK;
				return 0;
			}
			close(job->ptr.fd);
			/* finished */
		}
		break;
		case APE_SOCKET_JOB_SHUTDOWN:
			shutdown(socket->s.fd, 2);
			return 0;
		default:
			printf("[Job] :(\n");
			break;
		}

		job = job->next;
	}
	
	return 0;
	
}

static int ape_socket_queue_data(ape_socket *socket, 
		char *data, size_t len, int offset)
{
	ape_socket_jobs_t *job;
	ape_socket_packet_t *packets;
	ape_pool_list_t *list;
	
	/* socket is about to close, don't queue */
	if (socket->jobs.queue->flags & APE_SOCKET_JOB_SHUTDOWN /* TODO: replace this to get the last active */) {
		return 0;
	}

	job = ape_socket_job_get_slot(socket, APE_SOCKET_JOB_WRITEV);
	list = job->ptr.data;

	if (list == NULL) {
		list = ape_socket_new_packet_queue(8);
		job->ptr.data = list;
	}
	packets = (ape_socket_packet_t *)list->current;

	packets->pool.ptr.data = data;
	packets->len 	= len;
	packets->offset = offset;
	
	/* Always have spare slots */
	if (packets->pool.next == NULL) {
		ape_grow_pool(list, sizeof(ape_socket_packet_t), 8);
	}
	
	list->current = packets->pool.next;
	
	return 0;
}


static int ape_socket_queue_buffer(ape_socket *socket, buffer *b)
{
	return ape_socket_queue_data(socket, b->data, b->used, 0);
}


inline int ape_socket_accept(ape_socket *socket, ape_global *ape)
{
	int fd, sin_size = sizeof(struct sockaddr_in);
	struct sockaddr_in their_addr;
	ape_socket *client;
	
	while(1) { /* walk through backlog */
		fd = accept(socket->s.fd, 
			(struct sockaddr *)&their_addr,
			(unsigned int *)&sin_size);
			
		if (fd == -1) break; /* EAGAIN ? */

		client			= APE_socket_new(socket->states.proto, fd);

		client->callbacks 	= socket->callbacks; /* clients inherits server callbacks */
		
		client->states.state = APE_SOCKET_ST_ONLINE;
		client->states.type  = APE_SOCKET_TP_CLIENT;
				
		events_add(client->s.fd, client, EVENT_READ|EVENT_WRITE, ape);
		
		if (socket->callbacks.on_connect != NULL) {
			socket->callbacks.on_connect(client, ape);
		}
	}
	
	return 0;
}

/* Consume socket buffer */
inline int ape_socket_read(ape_socket *socket, ape_global *ape)
{
	ssize_t nread;
	
	do {
		/* TODO : avoid extra calling (avoid realloc) */
		buffer_prepare(&socket->data_in, 2048);

		nread = read(socket->s.fd, 
			socket->data_in.data + socket->data_in.used, 
			socket->data_in.size - socket->data_in.used);
			
		socket->data_in.used += ape_max(nread, 0);
		
	} while (nread > 0);

	if (socket->data_in.used != 0) {
		//buffer_append_char(&socket->data_in, '\0');

		if (socket->callbacks.on_read != NULL) {
			socket->callbacks.on_read(socket, ape);
		}
	
		socket->data_in.used = 0;
	}
	if (nread == 0) {
		if (socket->callbacks.on_disconnect != NULL) {
			socket->callbacks.on_disconnect(socket, ape);
		}

		APE_socket_destroy(socket, ape);
		
		return -1;
	}

	return socket->data_in.used;
}

int ape_socket_write_file(ape_socket *socket, 
		const char *file, ape_global *ape)
{
	int fd, nwrite = 0;
	off_t offset = 0;
	
	if ((fd = open(file, O_RDONLY)) == -1) {
		shutdown(socket->s.fd, 2);
		return 0;
	}
	
	do {
		PACK_TCP(socket->s.fd);
		nwrite = sendfile(socket->s.fd, fd, &offset, 2048);
		//printf("write %i\n", nwrite);
		if (nwrite == -1) {
			break;
		}
		FLUSH_TCP(socket->s.fd);
		//printf("write %i\n", nwrite);
	} while (nwrite > 0);
	
	close(fd);
	shutdown(socket->s.fd, 2);
	
	return 1;
}

static ape_socket_jobs_t *ape_socket_job_get_slot(ape_socket *socket, int type)
{
	ape_socket_jobs_t *jobs = socket->jobs.current;
	
	/* If we request a write job we can push the data to the iov list */
	if ((type == APE_SOCKET_JOB_WRITEV && 
			jobs->flags & APE_SOCKET_JOB_WRITEV) ||
			!(jobs->flags & APE_SOCKET_JOB_ACTIVE)) {
			
		jobs->flags |= APE_SOCKET_JOB_ACTIVE | type;
		return jobs;
	}
	
	/* Can't do anything after a shutdown */
	if (socket->jobs.current->flags & APE_SOCKET_JOB_SHUTDOWN) {
		return NULL;
	}
	
	jobs = (jobs == socket->jobs.queue ? 
		ape_grow_pool(&socket->jobs, sizeof(ape_socket_jobs_t), 2) : 
		jobs->next);

	socket->jobs.current = jobs;
	jobs->flags |= APE_SOCKET_JOB_ACTIVE | type;	

	return jobs;
}

static void ape_init_job_list(ape_pool_list_t *list, size_t n)
{
	ape_init_pool_list(list, sizeof(ape_socket_jobs_t), n);
}

static ape_socket_jobs_t *ape_socket_new_jobs_queue(size_t n)
{
	return (ape_socket_jobs_t *)ape_new_pool(sizeof(ape_socket_jobs_t), n);

}

static ape_pool_list_t *ape_socket_new_packet_queue(size_t n)
{
	return ape_new_pool_list(sizeof(ape_socket_packet_t), n);
}



