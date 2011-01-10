#include "ape_config.h"
#include "server.h"
#include <stdlib.h>
#define _GNU_SOURCE
#include <string.h>

static void ape_config_error(cfg_t *cfg, const char *fmt, va_list ap);

int ape_config_server_setup(cfg_t *conf, ape_server *server)
{
	
}

cfg_t *ape_read_config(const char *file, ape_global *ape)
{
	cfg_t *cfg;
	cfg_t *server;
	int i;
	char *ipport;
	
	cfg_opt_t server_opts[] =
	{
		CFG_INT("port", 6969, CFGF_NONE),
		CFG_END()
	};
	
	cfg_opt_t opts[] =
	{
		CFG_SEC("server", server_opts, CFGF_MULTI | CFGF_TITLE),
		CFG_END()
	};
	
	cfg = cfg_init(opts, CFGF_NOCASE);
	
	cfg_set_error_function(cfg, ape_config_error);
	
	if (cfg_parse(cfg, file) != CFG_SUCCESS) {
		cfg_free(cfg);
		return NULL;
	}
	
	for (i = 0; i < cfg_size(cfg, "server"); i++) {
		ape_server *aserver;
		char *sep, ip[16];
		uint16_t port;
		struct in_addr addr4;
		
		server = cfg_getnsec(cfg, "server", i);
		
		ipport = strdup(cfg_title(server));
		
		sep = strchr(ipport, ':');
		*sep = '\0';

		if (sep == ipport) {
			strcpy(ip, "0.0.0.0");
		} else if (inet_pton(AF_INET, ipport, &addr4) == 1) {
			strncpy(ip, ipport, 16);
		} else {
			goto error;
		}
		
		port = atoi(&sep[1]);
		if (port == 0 || port > 65535) {
			goto error;
		}
		
		free(ipport);
		
		if ((aserver = ape_server_init(port, ip, ape)) != NULL) {
			ape_config_server_setup(server, aserver);
		}
		
	}
	
	return cfg;

error:
	cfg_free(cfg);
	free(ipport);
	return NULL;		
}


static void ape_config_error(cfg_t *cfg, const char *fmt, va_list ap)
{
	printf("[Config error] ");
	if (cfg && cfg->filename && cfg->line) {
		printf("%s:%d ", cfg->filename, cfg->line);
	} else if (cfg && cfg->filename) {
		printf("%s ", cfg->filename);
	}
        vprintf(fmt, ap);
        printf("\n");
}


