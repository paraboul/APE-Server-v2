#ifndef __APE_CMD_H_
#define __APE_CMD_H_

#include <sys/types.h>
#include <stdint.h>

#include "common.h"

#include "ape_server.h"
#include "ape_json.h"

#define APE_CMD_REQ_USER     (1 << 0)
#define APE_CMD_FAILFATAL    (1 << 1)


typedef struct _ape_cmd_spec_t ape_cmd_spec_t;
typedef struct _ape_message_client_t ape_message_client_t;

struct _ape_cmd_spec_t
{
    const char      *name;
    void            (*call)(ape_message_client_t *, ape_global *);
    uint16_t        flags;
};

struct _ape_message_client_t
{
    uint32_t chl;
    ape_cmd_spec_t *cmd;
    ape_client *client;
    time_t time;
    
};

int APE_cmd_register(ape_cmd_spec_t *spec, ape_global *ape);
void ape_cmd_init_core(ape_global *ape);

int ape_cmd_process(json_item *head, ape_client *client, ape_global *ape);
void ape_cmd_process_multi(json_item *head, ape_client *client, ape_global *ape);


#define APE_CMD(name,call,flags)                                         \
    {name, call, flags}

#define APE_CMD_END APE_CMD(NULL,NULL,0)

#endif
