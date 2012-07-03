#include "ape_cmd.h"
#include "ape_json.h"
#include "ape_server.h"
#include "ape_hash.h"
#include "ape_user.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

static void ape_cmd__connect(ape_message_client_t *msg, ape_global *ape);

static ape_cmd_spec_t ape_cmd_core[] = {
    APE_CMD("connect", ape_cmd__connect, APE_CMD_FAILFATAL),
    APE_CMD_END
};

void ape_cmd_init_core(ape_global *ape)
{
    int i;
    
    for (i = 0; ape_cmd_core[i].name != NULL; i++) {
        printf("[Core] register cmd : \033[1m%s\033[0m\n", ape_cmd_core[i].name);
        APE_cmd_register(&ape_cmd_core[i], ape);
    }
}

int APE_cmd_register(ape_cmd_spec_t *spec, ape_global *ape)
{
     hashtbl_append(ape->hashs.cmds, spec->name, strlen(spec->name), spec);
     
     return 1;
}

ape_cmd_spec_t *APE_cmd_lookup(const char *name, int len, ape_global *ape)
{
    return (ape_cmd_spec_t *)hashtbl_seek(ape->hashs.cmds, name, len);
}

int ape_cmd_process(json_item *head, ape_client *client, ape_global *ape)
{
    int i;

    ape_message_client_t msg = {
        .cmd    = NULL,
        .client = client,
        .time   = time(NULL),
        .chl    = 0
    };

    for (i = 0; head != NULL; head = head->next, i++) {
        switch(i) {
        case 0: /* chl */
            if (head->type != JSON_T_INTEGER) {
                printf("BAD_CHL\n");
                return 0;
            }
            msg.chl = head->jval.vu.integer_value;
            
            break;
        case 1: /* params (optional) */
            break;
        case 2: /* cmd (optional) */
            printf("two?\n");
            if (head->type != JSON_T_STRING) {
                printf("BAD_CMD_PARAM\n");
                return 0;
            }
            if ((msg.cmd = APE_cmd_lookup(head->jval.vu.str.value,
                    head->jval.vu.str.length, ape)) == NULL) {
                printf("BAD_CMD\n");
                return 0;
            }
            if ((msg.cmd->flags & APE_CMD_REQ_USER) &&
                    client->user_session == NULL) {
                printf("NEED_AUTH\n");
                return 0;
            }
            
            break;
        default:
            break;
        }
    }
    
    if (msg.cmd != NULL) {
        msg.cmd->call(&msg, ape);
    }

    return 1;
}

void ape_cmd_process_multi(json_item *head, ape_client *client, ape_global *ape)
{

    if (head == NULL || head->jchild.child == NULL) {
        printf("JSON malformed?\n");
        return;
    }

    for (head = head->jchild.child; head != NULL; head = head->next) {
        if (head->jchild.type != JSON_C_T_ARR) {
            printf("cmd not an array\n");
            return;
        }
        
        if (ape_cmd_process(head->jchild.child, client, ape) == 0) {
            break;
        }
    }

}

static void ape_cmd__connect(ape_message_client_t *msg, ape_global *ape)
{
    if (msg->client->user_session) {
        /* Already connected */
        return;
    }
    
    msg->client->user_session = APE_user_session_new(APE_user_new(ape),
                                    msg->client, ape);
    
    
    printf("Got a connection - %s\n", msg->client->user_session->user->pipe->id.priv.str);

}

