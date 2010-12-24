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

#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sendfile.h>


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

static ape_socket_jobs_t *ape_socket_new_jobs(size_t n);

ape_socket *APE_socket_new(uint32_t pt, int from)
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

	proto = (pt == ((uint32_t)APE_SOCKET_PT_UDP) ? SOCK_DGRAM : SOCK_STREAM);
	
	if (sock == 0 && 
		(sock = socket(AF_INET /* TODO AF_INET6 */, proto, 0)) == -1 || 
		setnonblocking(sock) == -1) {
		return NULL;
	}

	ret 		= malloc(sizeof(*ret));
	ret->s.fd 	= sock;
	ret->s.type	= APE_SOCKET;
	ret->flags	= 0;
	
	APE_SOCKET_SET_BITS(ret->flags, APE_SOCKET_TP_UNKNOWN);
	APE_SOCKET_SET_BITS(ret->flags, APE_SOCKET_ST_PENDING);
	
	if (proto == SOCK_DGRAM) {
		APE_SOCKET_SET_BITS(ret->flags, APE_SOCKET_PT_UDP);
	} else {
		APE_SOCKET_SET_BITS(ret->flags, APE_SOCKET_PT_TCP);
	}
	
	
	ret->callbacks.on_read 		= NULL;
	ret->callbacks.on_disconnect 	= NULL;
	ret->callbacks.on_connect	= NULL;
	
	ret->remote_port = 0;

	buffer_init(&ret->data_in);
	buffer_init(&ret->data_out);
	
	ret->jobs.list = ape_socket_new_jobs(2);
	ret->jobs.last = &ret->jobs.list[1];

	ret->file_out.fd 	= 0;
	ret->file_out.offset 	= 0;
	
	return ret;
}

int APE_socket_listen(ape_socket *socket, uint16_t port, const char *local_ip, ape_global *ape)
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
		(APE_SOCKET_HAS_BITS(socket->flags, APE_SOCKET_PT_TCP) && /* only listen for STREAM socket */
		listen(socket->s.fd, APE_SOCKET_BACKLOG) == -1)) {
		
		close(socket->s.fd);
		
		return -1;
	}

	APE_SOCKET_SET_BITS(socket->flags, APE_SOCKET_TP_SERVER);
	APE_SOCKET_SET_BITS(socket->flags, APE_SOCKET_ST_ONLINE);
	
	events_add(socket->s.fd, socket, EVENT_READ|EVENT_WRITE, ape);

	return 0;
	
}

static int ape_socket_connect_ready_to_connect(const char *remote_ip, void *arg, int status, ape_global *ape)
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

	APE_SOCKET_SET_BITS(socket->flags, APE_SOCKET_TP_CLIENT);
	APE_SOCKET_SET_BITS(socket->flags, APE_SOCKET_ST_PROGRESS);	
	
	events_add(socket->s.fd, socket, EVENT_READ|EVENT_WRITE, ape);
	
	return 0;
	
}

int APE_socket_connect(ape_socket *socket, uint16_t port, const char *remote_ip_host, ape_global *ape)
{
	if (port == 0 || port > 65535) {
		APE_socket_destroy(socket, ape);
		return -1;
	}
	
	socket->remote_port = port;
	ape_gethostbyname(remote_ip_host, ape_socket_connect_ready_to_connect, socket, ape);
	
	return 0;
}

int APE_socket_write(ape_socket *socket, const char *data, size_t len, ape_global *ape)
{

	if (!APE_SOCKET_HAS_BITS(socket->flags, APE_SOCKET_ST_ONLINE) || len == 0) {
		return 0;
	}

	if (APE_SOCKET_HAS_BITS(socket->flags, APE_SOCKET_WOULD_BLOCK)) {
		//ape_socket_queue_data(socket, data, len);
		return 0;
	}		
}

int APE_socket_destroy(ape_socket *socket, ape_global *ape)
{
	buffer_delete(&socket->data_in);
	buffer_delete(&socket->data_out);

	APE_SOCKET_SET_BITS(socket->flags, APE_SOCKET_ST_OFFLINE);
	
	close(socket->s.fd);
	
	if (socket->jobs.last != NULL) {
		ape_socket_jobs_t *jobs = socket->jobs.list, *tJobs = NULL;
		
		while (jobs != NULL) {
			/* TODO : callback ? */
			if (jobs->start) {
				if (tJobs != NULL) {
					free(tJobs);
				}
				tJobs = jobs;
			}
			jobs = jobs->next;
		}
		if (tJobs != NULL) {
			free(tJobs);
		}
	}

	free(socket);
}

static ape_socket_jobs_t *ape_socket_new_jobs(size_t n)
{
	int i;
	ape_socket_jobs_t *jobs = malloc(sizeof(*jobs) * n);
	
	for (i = 0; i < n; i++) {
		jobs[i].next = (i == n-1 ? NULL : &jobs[i+1]); /* contiguous blocks */
		jobs[i].ptr = NULL;
		jobs[i].dowhat = APE_SOCKET_JOB_WRITEV;
		jobs[i].start = (i == 0);
	}
	
	return jobs;
}

inline static int ape_socket_queue_data(ape_socket *socket, const char *data, size_t len)
{
	if (socket->jobs.last->dowhat == APE_SOCKET_JOB_SHUTDOWN) { /* socket is about to close, don't queue */
		return 0;
	}
	if (socket->jobs.last->dowhat != APE_SOCKET_JOB_WRITEV) { /* cannot push the data to the last queued job (i.e. sendfile) */
		socket->jobs.last->next = ape_socket_new_jobs(1);
		socket->jobs.last 	= socket->jobs.last->next;
	}
	
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
		
		if (APE_SOCKET_HAS_BITS(socket->flags, APE_SOCKET_PT_UDP)) {
			client 		= APE_socket_new(APE_SOCKET_PT_UDP, fd);
		} else {
			client 		= APE_socket_new(APE_SOCKET_PT_TCP, fd);
		}
		
		client->flags		= 0;
		client->callbacks 	= socket->callbacks; /* clients inherits server callbacks */
		
		APE_SOCKET_SET_BITS(client->flags, APE_SOCKET_ST_ONLINE);
		APE_SOCKET_SET_BITS(client->flags, APE_SOCKET_TP_CLIENT);
				
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

int ape_socket_write_file(ape_socket *socket, const char *file, ape_global *ape)
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




