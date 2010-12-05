#include "ape_config.h"

cfg_t *ape_read_config(const char *file)
{
	cfg_t *cfg;
	cfg_t *server;
	
	cfg_opt_t greet_opts[] =
	{
		CFG_INT("port", 6969, CFGF_NONE),
		CFG_END()
	};
	
	cfg_opt_t opts[] =
	{
		CFG_SEC("server", greet_opts, CFGF_MULTI | CFGF_TITLE),
		CFG_END()
	};
	
	cfg = cfg_init(opts, CFGF_NONE);
	if (cfg_parse(cfg, file) != CFG_SUCCESS) {
		printf("parse error\n");
		return;
	}
	
	server = cfg_getsec(cfg, "server");
	
	printf("port : %i\n", cfg_getint(server, "port"));
	
	printf("success\n");

}
