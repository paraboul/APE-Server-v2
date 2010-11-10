#include "common.h"
#include "events.h"


/*int events_add(struct _fdevent *ev, int fd, int bitadd)
{
	if (ev->add(ev, fd, bitadd) == -1) {
		return -1;
	}
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

