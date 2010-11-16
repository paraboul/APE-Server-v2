#include "../core/APEapi.h"

extern ape_module_t ape_inotify_module;

ape_module_t *ape_modules[] = {
	&ape_inotify_module,
	NULL
};

