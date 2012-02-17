#ifndef __APE_PIPE_H
#define __APE_PIPE_H

#include "common.h"

typedef struct _ape_pipe ape_pipe;

typedef enum {
    APE_PIPE_PUB_ID,
    APE_PIPE_PRIV_ID
} ape_pipe_id_t;

struct _ape_pipe
{

    struct {
        struct {
            char str[16];
            uint64_t bin;
        } priv;
        struct {
            char str[16];
            uint64_t bin;
        } pub;
    } id;
    
    union {
        struct _ape_user *user;
        void *ctx;
    } link;
    
};

ape_pipe *APE_pipe_new(ape_global *ape);

#endif
