#include "common.h"
#include "ape_user.h"
#include "ape_pipe.h"
#include "ape_array.h"


ape_user *APE_user_new(ape_global *ape)
{
    ape_user *user = malloc(sizeof(*user));
    
    user->pipe = APE_pipe_new(ape);
    user->extend = ape_array_new(8);
    
    user->pipe->link.user = user;
    
    return user;
}

