#include "common.h"
#include "ape_buffer.h"
#include "ape_events.h"

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "ape_hash.h"
#include "ape_socket.h"
#include "ape_events_loop.h"
#include "ape_server.h"
#include "ape_dns.h"
#include "ape_modules.h"
#include "ape_config.h"
#include "ape_pool.h"
#include "ape_array.h"
#include "ape_extend.h"
#include "ape_ssl.h"

//gcc -g *.c ../modules/*.c -I../core/ -I../../deps/ -I../../deps/mozilla/js/src/dist/include -I/usr/include/ ../../deps/c-ares/.libs/libcares.a ../../deps/confuse-2.7/src/.libs/libconfuse.a ../../deps/mozilla/js/src/libjs_static.a -lrt -lstdc++

int ape_running = 0;
ape_module_t *ape_modules[] = {
	//&ape_inotify_module,
	NULL,
	NULL
};


static void signal_handler(int sign)
{
	ape_running = 0;
	printf("[Quit] Shutting down...\n");
}

static int inc_rlimit(int nofile)
{
	struct rlimit rl;

	rl.rlim_cur = nofile;
	rl.rlim_max = nofile;

	return setrlimit(RLIMIT_NOFILE, &rl);
}

static ape_global *ape_init()
{
    ape_global *ape;
    struct _fdevent *fdev;

    if ((ape = malloc(sizeof(*ape))) == NULL) return NULL;

	inc_rlimit(64000);

    signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, &signal_handler);
	signal(SIGTERM, &signal_handler);
	
    fdev = &ape->events;
    fdev->handler = EVENT_UNKNOWN;
    #ifdef USE_EPOLL_HANDLER
    fdev->handler = EVENT_EPOLL;
    #endif
    #ifdef USE_KQUEUE_HANDLER
    fdev->handler = EVENT_KQUEUE;
    #endif

    ape->basemem    = APE_BASEMEM;
    ape->is_running = 1;
	
	ape_ssl_init();
	
    if (ape_dns_init(ape) != 0) {
        goto error;
    }
    events_init(ape);

    ape->seed = _ape_seed = time(NULL) ^ (getpid() << 16);

    ape->hashs.servers = hashtbl_init();

    if ((ape->conf = ape_read_config("../../etc/ape.conf", ape)) == NULL) {
        goto error;
    }

    ape->extend = ape_array_new(8);
	
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
    for (z = 0; ape_modules[z]; z++) {
        if (ape_modules[z]->ape_module_loaded && ape_modules[z]->ape_module_loaded(ape) == 0) {
            ;
        }
    }
}

static void ape_unload_modules(ape_global *ape)
{
    int z;

    for (z = 0; ape_modules[z]; z++) {
        if (ape_modules[z]->ape_module_destroy && ape_modules[z]->ape_module_destroy(ape) == 0) {
            printf("[Module] %s unloaded\n", ape_modules[z]->name);
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

    /*printf("ape addr : %p\n", ape);

    ape_add_property(ape->extend, "foo", ape);

    printf("Get addr %p\n", ape_get_property(ape->extend, "foo", 3));*/
    ape_running = 1;
    events_loop(ape);
    
    ape_unload_modules(ape);

    return 0;
}

// vim: ts=4 sts=4 sw=4 et

