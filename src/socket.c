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

#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>


/* 
	Use only one syscall (ioctl) if FIONBIO is defined
	It behave the same for socket file descriptor to use either ioctl(...FIONBIO...) or fcntl(...O_NONBLOCK...)
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


void *ape_socket_new(uint16_t port, const char *ip, ape_socket_proto pt)
{
	int sock, proto = SOCK_STREAM;
	struct sockaddr_in addr;
	
#ifdef __WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 2, 2 );

	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		return NULL;
	}	
#endif
	if (port == 0 || port > 65535) {
		return NULL;
	}
	
	switch(pt) {
		case APE_SOCKET_UDP:
			proto = SOCK_DGRAM;
			break;
		case APE_SOCKET_TCP:
		default:
			proto = SOCK_STREAM;
	}
	
	if ((sock = socket(AF_INET /* TODO AF_INET6 */, proto, 0)) == -1) {
		return NULL;
	}
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	memset(&(addr.sin_zero), '\0', 8);
	
	setnonblocking(sock);
	
	
	return NULL;
}

int ape_socket_listen()
{

}

int ape_socket_connect()
{

}

int ape_socket_destroy()
{

}





