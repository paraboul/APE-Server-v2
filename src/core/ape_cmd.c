#include "ape_cmd.h"
#include "ape_json.h"
#include "ape_server.h"
#include "ape_hash.h"

#include <stdlib.h>
#include <string.h>

static ape_cmd_spec_t ape_cmd_core[] = {
    APE_CMD("connect", NULL, APE_CMD_FAILFATAL),
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
    ape_message_t msg = {
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
                break;
            }
            msg.chl = head->jval.vu.integer_value;
            
            break;
        case 1: /* params */
            break;
        case 2:
            
            break;
        default:
            break;
        }
    }

}

void ape_cmd_process_multi(json_item *head, ape_client *client, ape_global *ape)
{
    ape_cmd_spec_t *cmd;
    
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
