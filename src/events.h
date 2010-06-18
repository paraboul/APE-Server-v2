#ifndef _EVENTS_H_
#define _EVENTS_H_

/* Generics flags */
#define EVENT_READ 0x01
#define EVENT_WRITE 0x02

typedef enum {
	EVENT_UNKNOWN,
	EVENT_EPOLL, 	/* Linux */
	EVENT_KQUEUE, 	/* BSD */
	EVENT_DEVPOLL,	/* Solaris */
	EVENT_POLL,	/* POSIX */
	EVENT_SELECT	/* Generic (Windows) */
} fdevent_handler_t;

struct _fdevent {
	/* Common values */
	int *basemem;
	/* Interface */
	int (*add)(struct _fdevent *, int, int);
	int (*poll)(struct _fdevent *, int);
	int (*get_current_fd)(struct _fdevent *, int);
	void (*growup)(struct _fdevent *);
	int (*revent)(struct _fdevent *, int);
	int (*reload)(struct _fdevent *);
	
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

#endif


