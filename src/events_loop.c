#include "common.h"
#include "events.h"


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
			int active_fd = events_get_current_fd(&ape->events, i);
			
			printf("Event on %i\n", active_fd);
		}

	}
}
