#ifndef __APE_MODULES_H_
#define __APE_MODULES_H_

#define APE_EVENT(evname, arg...) \
    { \
        int __ape_event_loop; \
        for (__ape_event_loop = 0; ape_modules[__ape_event_loop]; __ape_event_loop++) { \
            if (ape_modules[__ape_event_loop]->ape_module_##evname && ape_modules[__ape_event_loop]->ape_module_##evname(arg) == 0) { \
                ; \
            } \
        } \
    }

#endif
