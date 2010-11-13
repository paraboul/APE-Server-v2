#include "common.h"
#include "events.h"
#include "socket.h"

void events_loop(ape_global *ape)
{
	int nfd;
	
	while(ape->is_running) {
		int i;
		
		if ((nfd = events_poll(&ape->events, 1)) == -1) {
			printf("events error\n");
			continue;
		}
		
		for (i = 0; i < nfd; i++) {
			int fd = events_get_current_fd(&ape->events, i);
			
			switch(ape->events.fds[fd].type) {
				case APE_SOCKET:
					switch(((ape_socket *)ape->events.fds[fd].data)->type) {
						case APE_SOCKET_SERVER:
							ape_socket_accept(((ape_socket *)ape->events.fds[fd].data), ape);
							printf("Event on server\n");
							break;
						case APE_SOCKET_CLIENT:
							printf("Event on client\n");
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
