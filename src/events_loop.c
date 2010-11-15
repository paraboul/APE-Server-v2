#include "common.h"
#include "events.h"
#include "socket.h"

void events_loop(ape_global *ape)
{
	int nfd, fd, bitev;
	void *attach;
	
	while(ape->is_running) {
		int i;
		
		if ((nfd = events_poll(&ape->events, 1)) == -1) {
			printf("events error\n");
			continue;
		}
		
		for (i = 0; i < nfd; i++) {
			attach 	= events_get_current_fd(&ape->events, i);
			bitev 	= events_revent(&ape->events, i);
			fd	= ((ape_fds *)attach)->fd; /* assuming that ape_fds is the first member */
			
			switch(((ape_fds *)attach)->type) {
			
			case APE_SOCKET:
				switch(APE_SOCKET(attach)->type) {
				
				case APE_SOCKET_SERVER:
					if (bitev & EVENT_READ) {
						if (APE_SOCKET(attach)->proto == APE_SOCKET_TCP) {
							ape_socket_accept(APE_SOCKET(attach), ape);
						} else {							
							printf("read on UDP\n");
						}
					}
					break;
				case APE_SOCKET_CLIENT:
					if (bitev & EVENT_WRITE) {
						//printf("Write on client\n");
					}
					if (bitev & EVENT_READ) {
						ape_socket_read(APE_SOCKET(attach), ape);
					}
					
					break;
				case APE_SOCKET_UNKNOWN:
					break;
				}
				
				break;
			case APE_FILE:
				break;
			case APE_DELEGATE:
				printf("We got something in delegate %i %i\n", fd, bitev);
				ares_process_fd(ape->dns.channel, (bitev & EVENT_READ ? fd : ARES_SOCKET_BAD), (bitev & EVENT_WRITE ? fd : ARES_SOCKET_BAD));
				break;
			}
		}

	}
}
