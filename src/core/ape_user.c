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

ape_user_session *APE_user_session_new(ape_user *user,
        ape_client *client, ape_global *ape)
{
    ape_user_session *session = malloc(sizeof(*session));
    
    session->user   = user;
    session->client = client;
    
    session->time.idle    = time(NULL);
    session->time.connect = session->time.idle;
    
    return session;
}

