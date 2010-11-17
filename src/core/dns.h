#ifndef __APE_DNS_H_
#define __APE_DNS_H_

#include "common.h"

typedef int (*ape_gethostbyname_callback)(const char *ip, void *arg, int status, ape_global *ape);


struct _ares_sockets {
	_APE_FD_DELEGATE_TPL
};


int ape_dns_init(ape_global *ape);
void ape_gethostbyname(const char *host, ape_gethostbyname_callback callback, void *arg, ape_global *ape);

#endif
