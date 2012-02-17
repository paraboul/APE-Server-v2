#ifndef __APE_USER_H
#define __APE_USER_H

#include "ape_pipe.h"
#include "ape_extend.h"

typedef struct _ape_user ape_user;

struct _ape_user
{
    ape_pipe *pipe;
    ape_extend_t *extend;
    
};

ape_user *APE_user_new(ape_global *ape);

#endif

