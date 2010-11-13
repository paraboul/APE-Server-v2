#include "common.h"
#include "events.h"
#include "socket.h"

#include <stdlib.h>


int events_add(void *fd_data, ape_fds_t type, int bitadd, ape_global *ape)
{
	struct _fdevent *ev = &ape->events;
	int fd;

	
	switch(type) {
		case APE_SOCKET:
			fd = ((ape_socket *)fd_data)->fd;
			break;
		case APE_FILE:
			break;
	}
	
	if (ev->add(ev, fd, bitadd) == -1) {
		return -1;
	}
	
	ev->fds[fd].data = fd_data;
	ev->fds[fd].type = type;
	
	return 1;
}

inline int events_poll(struct _fdevent *ev, int timeout_ms)
{
	int nfds;
	
	if ((nfds = ev->poll(ev, timeout_ms)) == -1) {
		return -1;
	}
	
	return nfds;
}


inline int events_get_current_fd(struct _fdevent *ev, int i)
{
	return ev->get_current_fd(ev, i);
}

/*
void events_growup(struct _fdevent *ev)
{
	ev->growup(ev);
}

int events_revent(struct _fdevent *ev, int i)
{
	return ev->revent(ev, i);
}

int events_reload(struct _fdevent *ev)
{
	return ev->reload(ev);
}
*/

int events_init(ape_global *ape)
{
	ape->events.basemem = &ape->basemem;
	ape->events.fds = malloc(sizeof(*ape->events.fds) * ape->basemem);


	switch(ape->events.handler) {
		case EVENT_EPOLL:
			return event_epoll_init(&ape->events);
			break;
		case EVENT_KQUEUE:
			return event_kqueue_init(&ape->events);
			break;
		default:
			break;
	}
	
	return -1;
}

