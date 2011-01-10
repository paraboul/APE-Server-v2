#include "common.h"
#include "ape_buffer.h"
#include "events.h"

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include "ape_hash.h"
#include "socket.h"
#include "events_loop.h"
#include "server.h"
#include "dns.h"
#include "modules.h"
#include "ape_config.h"
#include "ape_pool.h"
#include "ape_array.h"

//gcc -g *.c ../modules/*.c -I../core/ -I../../deps/ -I/usr/include/ ../../deps/c-ares/.libs/libcares.a ../../deps/confuse-2.7/src/.libs/libconfuse.a ../../deps/jsapi/src/libjs_static.a -lrt -lstdc++

static ape_global *ape_init()
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
	fdev->handler = EVENT_ KQUEUE;
	#endif
	
	ape->basemem 	= APE_BASEMEM;
	ape->is_running = 1;

	if (ape_dns_init(ape) != 0) {
		goto error;
	}
	events_init(ape);
	
	ape->seed = _ape_seed = time(NULL) ^ (getpid() << 16);
	
	if ((ape->conf = ape_read_config("../../etc/ape.conf", ape)) == NULL) {
		goto error;
	}

	return ape;
	
error:
	
	free(ape);
	
	return NULL;
}

static void ape_load_modules(ape_global *ape)
{
	int z;
	
	for (z = 0; ape_modules[z]; z++) {
		if (ape_modules[z]->ape_module_init(ape) == 0) {
			printf("[Module] %s loaded\n", ape_modules[z]->name);
		} else {
			printf("[Module] Failed to load %s\n", ape_modules[z]->name);
		}
	}
}

int main(int argc, char **argv)
{
	ape_global *ape;

	printf("    _    ____  _____   ____    ___  \n");
	printf("   / \\  |  _ \\| ____| |___ \\  / _ \\ \n");
	printf("  / _ \\ | |_) |  _|     __) || | | |\n");
	printf(" / ___ \\|  __/| |___   / __/ | |_| |\n");
	printf("/_/   \\_\\_|   |_____| |_____(_)___/ \n\t   Async Push Engine (%s)\n\n", __REV);
	printf("Build   : %s %s\n", __DATE__, __TIME__);
	printf("Author  : Anthony Catel (a.catel@weelya.com)\n\n");

	if ((ape = ape_init()) == NULL) {
		printf("Failed to initialize APE\n");
		exit(1);
	}

	
	ape_load_modules(ape);

	events_loop(ape);

	return 0;
}

