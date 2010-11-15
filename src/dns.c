#include <c-ares/ares.h>
#include <netdb.h>
#include <stdlib.h>
#include "common.h"
#include "dns.h"
#include "events.h"

/* gcc *.c -I../deps/ ../deps/c-ares/.libs/libcares.a -lrt */

struct _ape_dns_cb_argv {
	ape_global *ape;
	ape_gethostbyname_callback callback;
};

static void ares_socket_cb(void *data, int s, int read, int write)
{
	ape_global *ape = data;
	
	//events_add(&s, APE_DELEGATE, EVENT_READ|EVENT_WRITE, NULL, ape);
	
	printf("Socket %i %i %i\n", s, read, write);
}

int ape_dns_init(ape_global *ape)
{
	struct ares_options opt;
	int optmask = 0;
	
	if (ares_library_init(ARES_LIB_INIT_ALL) != 0) {
		return -1;
	}
	
	opt.sock_state_cb 	= ares_socket_cb;
	opt.sock_state_cb_data 	= ape;
	
	if (ares_init_options(&ape->dns_channel, &opt, 0x00 | ARES_OPT_SOCK_STATE_CB) != ARES_SUCCESS) {
		return -1;
	}
}

void ares_gethostbyname_cb(void *arg, int status, int timeout, struct hostent *host)
{
	struct _ape_dns_cb_argv *params = arg;
	char ret[46];
	
	if (status == ARES_SUCCESS) {
		inet_ntop(host->h_addrtype, *host->h_addr_list, ret, sizeof(ret)); /* only return the first h_addr_list element */
		params->callback(ret);
	}
	
	free(params);
}

void ape_gethostbyname(const char *host, ape_gethostbyname_callback callback, ape_global *ape)
{
	struct in_addr addr4;
	
	if (inet_pton(AF_INET, host, &addr4) == 1) {
		callback(host);
	} else {
		struct _ape_dns_cb_argv *cb 	= malloc(sizeof(*cb));
		cb->ape 			= ape;
		cb->callback 			= callback;

		ares_gethostbyname(ape->dns_channel, host, AF_INET, ares_gethostbyname_cb, cb);

	}
}

