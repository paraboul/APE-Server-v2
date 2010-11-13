#include "common.h"
#include "buffer.h"
#include "string.h"
#include "events.h"

#include <stdio.h>
#include <signal.h>

#include "hash.h"
#include "socket.h"
#include "events_loop.h"

ape_global *ape_init()
{
	ape_global *ape;
	struct _fdevent *fdev;
	
	if ((ape = malloc(sizeof(*ape))) == NULL) return NULL;
		
	signal(SIGPIPE, SIG_IGN);
	
	fdev = &ape->events;
	fdev->handler = EVENT_UNKNOWN;
	#ifdef USE_EPOLL_HANDLER
	fdev->handler = EVENT_EPOLL;
	#endif
	#ifdef USE_KQUEUE_HANDLER
	fdev->handler = EVENT_KQUEUE;
	#endif
	
	ape->basemem 	= APE_BASEMEM;
	ape->is_running = 1;
	
	events_init(ape);
	
	return ape;
}

int main(int argc, char **argv)
{
	ape_global *ape;
	uint64_t h;
	ape_socket *sock;
	int z = 0;
	
	if ((ape = ape_init()) == NULL) {
		printf("Failed to allocate APE object\n");
		exit(1);
	}
	
	printf("    _    ____  _____   ____    ___  \n");
	printf("   / \\  |  _ \\| ____| |___ \\  / _ \\ \n");
	printf("  / _ \\ | |_) |  _|     __) || | | |\n");
	printf(" / ___ \\|  __/| |___   / __/ | |_| |\n");
	printf("/_/   \\_\\_|   |_____| |_____(_)___/ \n\t   Async Push Engine (%s)\n\n", __REV);
	printf("Build   : %s %s\n", __DATE__, __TIME__);
	printf("Author  : Anthony Catel (a.catel@weelya.com)\n\n");
	
	#if 0
	h = hash("fop", 3, 0);
	
	printf("arch : %llx\n", h);
	printf("arch : %x\n", h >> 32);
	printf("Hash : %x\n", 0);
	#endif
	
	sock = APE_socket_new(APE_SOCKET_TCP, 0);
	APE_socket_listen(sock, 6969, "127.0.0.1", ape);
	
	events_loop(ape);
	
	return 0;
}

