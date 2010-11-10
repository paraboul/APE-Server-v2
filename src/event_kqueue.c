/* event_kqueue.c */

#include "common.h"
#include "events.h"
#include <sys/time.h>
#include <time.h>

#include <string.h>
#include <stdlib.h>

#ifdef USE_KQUEUE_HANDLER
static int event_kqueue_add(struct _fdevent *ev, int fd, int bitadd)
{

	return 1;
}

static int event_kqueue_poll(struct _fdevent *ev, int timeout_ms)
{
	int nfds = 0;
	
	return nfds;
}

static int event_kqueue_get_fd(struct _fdevent *ev, int i)
{
	return 0;
}

static void event_kqueue_growup(struct _fdevent *ev)
{
	//ev->events = realloc(ev->events, sizeof(struct epoll_event) * (*ev->basemem));
}

static int event_kqueue_revent(struct _fdevent *ev, int i)
{
	int bitret = 0;
	
	return bitret;
}


int event_kqueue_reload(struct _fdevent *ev)
{
	int nfd = 0;
	
	return 1;
}

int event_kqueue_init(struct _fdevent *ev)
{
	if ((ev->kq_fd = kqueue()) == -1) {
		return 0;
	}

	ev->events = malloc(sizeof(struct kevent) * (*ev->basemem * 2));
	memset(ev->events, 0, sizeof(struct kevent) * (*ev->basemem * 2));
	
	ev->add = event_kqueue_add;
	ev->poll = event_kqueue_poll;
	ev->get_current_fd = event_kqueue_get_fd;
	ev->growup = event_kqueue_growup;
	ev->revent = event_kqueue_revent;
	ev->reload = event_kqueue_reload;
	
	return 1;
}

#else
int event_kqueue_init(struct _fdevent *ev)
{
	return 0;
}
#endif


