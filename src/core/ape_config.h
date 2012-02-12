#ifndef _APE_CONFIG_H
#define _APE_CONFIG_H

#include <confuse-2.7/src/confuse.h>

#include "common.h"

cfg_t *ape_read_config(const char *file, ape_global *ape);


typedef struct _ape_cfg_server {
    char ip[16];
    
    struct {
        int enabled;
        char *cert_path;
    } SSL;
    
    char *root;
    char **hostnames;
    
    uint16_t port;
    
} ape_cfg_server_t;

#endif

// vim: ts=4 sts=4 sw=4 et

