#include "../core/APEapi.h"

extern ape_module_t ape_inotify_module;
extern ape_module_t ape_jsapi_module;

ape_module_t *ape_modules[] = {
	//&ape_inotify_module,
	&ape_jsapi_module,
	NULL
};

