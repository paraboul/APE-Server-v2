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


ape_socket *APE_socket_new(ape_socket_proto pt, int from)
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

	switch(pt) {
	
	case APE_SOCKET_UDP:
		proto = SOCK_DGRAM;
		break;
	case APE_SOCKET_TCP:
	default:
		proto = SOCK_STREAM;
	}
	
	if (sock == 0 && (sock = socket(AF_INET /* TODO AF_INET6 */, proto, 0)) == -1) {
		return NULL;
	}

	if (setnonblocking(sock) == -1) {
		return NULL;
	}

	ret 		= malloc(sizeof(*ret));
	ret->s.fd 	= sock;
	ret->s.type	= APE_SOCKET;
	ret->type 	= APE_SOCKET_UNKNOWN;
	ret->state	= APE_SOCKET_PENDING;
	ret->proto	= pt;

	ret->callbacks.on_read 		= NULL;
	ret->callbacks.on_disconnect 	= NULL;
	ret->callbacks.on_connect	= NULL;
	
	ret->remote_port = 0;

	buffer_init(&ret->data_in);
	buffer_init(&ret->data_out);
	
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
		(socket->proto == APE_SOCKET_TCP && /* only listen for STREAM socket */
		listen(socket->s.fd, APE_SOCKET_BACKLOG) == -1)) {
		
		close(socket->s.fd);
		
		return -1;
	}
	
	socket->type  = APE_SOCKET_SERVER;
	socket->state = APE_SOCKET_ONLINE;
	
	events_add(socket->s.fd, socket, EVENT_READ|EVENT_WRITE, ape);
	
	return 0;
	
}

static int ape_socket_connect_ready_to_connect(const char *remote_ip, void *arg, int status, ape_global *ape)
{
	ape_socket *socket = arg;
	struct sockaddr_in addr;
	
	if (status != ARES_SUCCESS) {
		APE_socket_destroy(socket);
		return -1;
	}
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(socket->remote_port);
	addr.sin_addr.s_addr = inet_addr(remote_ip);
	memset(&(addr.sin_zero), '\0', 8);	
	
	if (connect(socket->s.fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == 0 || 
		errno != EINPROGRESS) {
		
		APE_socket_destroy(socket);
		return -1;
	}
	socket->type  = APE_SOCKET_CLIENT;
	socket->state = APE_SOCKET_PROGRESS;
	
	events_add(socket->s.fd, socket, EVENT_READ|EVENT_WRITE, ape);
	
	return 0;
	
}

int APE_socket_connect(ape_socket *socket, uint16_t port, const char *remote_ip_host, ape_global *ape)
{
	if (port == 0 || port > 65535) {
		APE_socket_destroy(socket);
		return -1;
	}
	
	socket->remote_port = port;
	ape_gethostbyname(remote_ip_host, ape_socket_connect_ready_to_connect, socket, ape);
	
	return 0;
}

int APE_socket_destroy(ape_socket *socket)
{
	buffer_delete(&socket->data_in);
	buffer_delete(&socket->data_out);

	close(socket->s.fd);
	
	free(socket);
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
		
		client 			= APE_socket_new(socket->proto, fd);
		client->type 		= APE_SOCKET_CLIENT;
		client->state		= APE_SOCKET_ONLINE;
		client->callbacks 	= socket->callbacks; /* clients inherits server callbacks */
		
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
		APE_socket_destroy(socket);
		
		return 0;
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




