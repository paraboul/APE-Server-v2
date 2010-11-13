#include "common.h"
#include "events.h"
#include "socket.h"

void events_loop(ape_global *ape)
{
	int nfd, fd, bitev;
	
	while(ape->is_running) {
		int i;
		
		if ((nfd = events_poll(&ape->events, 1)) == -1) {
			printf("events error\n");
			continue;
		}
		
		for (i = 0; i < nfd; i++) {
			fd 	= events_get_current_fd(&ape->events, i);
			bitev 	= events_revent(&ape->events, i);
			
			switch(ape->events.fds[fd].type) {
			
			case APE_SOCKET:
				switch(APE_SOCKET(fd, ape)->type) {
				
				case APE_SOCKET_SERVER:
					if (bitev & EVENT_READ) {
						if (APE_SOCKET(fd, ape)->proto == APE_SOCKET_TCP) {
							ape_socket_accept(APE_SOCKET(fd, ape), ape);
						} else {							
							printf("read on UDP\n");
						}
					}
					break;
				case APE_SOCKET_CLIENT:
					if (bitev & EVENT_WRITE) {
						printf("Write on client\n");
					}
					if (bitev & EVENT_READ) {
						ape_socket_read(APE_SOCKET(fd, ape), ape);
					}
					
					break;
				case APE_SOCKET_UNKNOWN:
					break;
				}
				
				break;
			case APE_FILE:
				break;
			}
		}

	}
}
