#ifndef __APE_USER_H
#define __APE_USER_H

#include "ape_pipe.h"
#include "ape_extend.h"
#include "ape_server.h"

#include <sys/time.h>

typedef struct _ape_user ape_user;
typedef struct _ape_user_session ape_user_session;

struct _ape_user
{
    ape_pipe *pipe;
    ape_extend_t *extend;
};

struct _ape_user_session
{
    struct {
        time_t idle;
        time_t connect;
    } time;
    
    ape_user *user;
    ape_client *client;
};

ape_user *APE_user_new(ape_global *ape);
ape_user_session *APE_user_session_new(ape_user *user,
        ape_client *client, ape_global *ape);

#endif

