#ifndef __APE_EVENTS_H_
#define __APE_EVENTS_H_

#include "config.h"

#include "common.h"

#ifdef USE_KQUEUE_HANDLER
#include <sys/event.h>
#endif
#ifdef USE_EPOLL_HANDLER
#include <sys/epoll.h>
#endif

/* Generics flags */
#define EVENT_READ 0x01
#define EVENT_WRITE 0x02

#define _APE_FD_DELEGATE_TPL  \
	ape_fds s; \
	void (*on_io)(int fd, int ev, ape_global *ape);

typedef enum {
	EVENT_UNKNOWN,
	EVENT_EPOLL, 	/* Linux */
	EVENT_KQUEUE, 	/* BSD */
	EVENT_DEVPOLL,	/* Solaris */
	EVENT_POLL,	/* POSIX */
	EVENT_SELECT	/* Generic (Windows) */
} fdevent_handler_t;

typedef enum {
	APE_SOCKET,
	APE_FILE,
	APE_DELEGATE
} ape_fds_t;

typedef struct { /* Do not store this. Address may changes */
	int fd;
	ape_fds_t type;
} ape_fds;

struct _ape_fd_delegate {
	_APE_FD_DELEGATE_TPL
};

struct _fdevent {
	/* Common values */
	int *basemem;
	//ape_fds *fds;
	
	/* Interface */
	int (*add)		(struct _fdevent *, int, int, void *);
	int (*poll)		(struct _fdevent *, int);
	void *(*get_current_fd)	(struct _fdevent *, int);
	void (*growup)		(struct _fdevent *);
	int (*revent)		(struct _fdevent *, int);
	int (*reload)		(struct _fdevent *);
	
	/* Specifics values */
#ifdef USE_KQUEUE_HANDLER
	struct kevent *events;
	int kq_fd;
#endif
#ifdef USE_EPOLL_HANDLER
	struct epoll_event *events;
	int epoll_fd;
#endif
	
	fdevent_handler_t handler;
};

int events_init(ape_global *ape);
int events_add(int fd, void *attach, int bitadd, ape_global *ape);
inline void *events_get_current_fd(struct _fdevent *ev, int i);
inline int events_poll(struct _fdevent *ev, int timeout_ms);

int event_kqueue_init(struct _fdevent *ev);
int event_epoll_init(struct _fdevent *ev);

#endif


