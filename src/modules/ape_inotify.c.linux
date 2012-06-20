#include <APEapi.h>
#include <ape_events.h>

#include <stdlib.h>
#include <sys/inotify.h>

struct _inotify_sockets {
	_APE_FD_DELEGATE_TPL
} insocket; /* /!\ not reentrant */


static void inotify_io(int fd, int ev, ape_global *ape)
{
	int nread = 0, cread = 0, size = 1024;
	void *ievent = malloc(sizeof(*ievent) * size);
	
	do {
		if (size - nread < 512) {
			ievent = realloc(ievent, size*2);
			size *= 2;
		}
		cread = read(fd, ievent+nread, size-nread);
		if (cread > 0) nread += cread;
		
	} while(cread > 0);
	
	for (cread = 0; cread < nread;) {
 		/* jump to the next element */	
		struct inotify_event *cevent = ievent + cread;
		/* the size of the last member (name) depends on 'len' */
		cread += sizeof(struct inotify_event) + cevent->len;
	}
	
	free(ievent);
}

static int ape_module_inotify_init(ape_global *ape)
{
	if ((insocket.s.fd = inotify_init1(0x00 | IN_NONBLOCK)) == -1) {
		return -1;
	}
	
	insocket.s.type  = APE_DELEGATE;
	insocket.on_io	 = inotify_io;
	
	events_add(insocket.s.fd, &insocket, EVENT_READ, ape);

	inotify_add_watch(insocket.s.fd, "/home/para/dev/", IN_ACCESS|IN_OPEN);
	
	return 0;
}

ape_module_t ape_inotify_module = {
	"APE inotify",
	ape_module_inotify_init,
	/*ape_module_inotidy_finish*/
};

