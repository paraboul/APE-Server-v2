#include "common.h"
#include "buffer.h"
#include "string.h"
#include "events.h"

#include <stdio.h>
#include <signal.h>


ape_global *ape_init()
{
	ape_global *ape = malloc(sizeof(*ape));
	struct _fdevent *fdev = &ape->events;
	
	signal(SIGPIPE, SIG_IGN);
		
	ape->basemem = APE_BASEMEM;
		
	fdev->handler = EVENT_UNKNOWN;

	#ifdef USE_EPOLL_HANDLER
	fdev->handler = EVENT_EPOLL;
	#endif
	#ifdef USE_KQUEUE_HANDLER
	fdev->handler = EVENT_KQUEUE;
	#endif
}

int main(int argc, char **argv)
{
	ape_global *ape;
	ape = ape_init();

	
	printf("    _    ____  _____   ____    ___  \n");
	printf("   / \\  |  _ \\| ____| |___ \\  / _ \\ \n");
	printf("  / _ \\ | |_) |  _|     __) || | | |\n");
	printf(" / ___ \\|  __/| |___   / __/ | |_| |\n");
	printf("/_/   \\_\\_|   |_____| |_____(_)___/ \n\t   AJAX Push Engine\n\n");
	printf("Build   : %s %s\n", __DATE__, __TIME__);
	printf("Author  : Anthony Catel (a.catel@weelya.com)\n\n");	
}

