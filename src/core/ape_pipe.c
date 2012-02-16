#include "common.h"
#include "ape_pipe.h"
#include "ape_hash.h"
#include "ape_base64.h"

static uint64_t ape_pipe_gen_id(ape_pipe_id_t type, ape_global *ape)
{
    ape_htable_t *h;
    uint64_t ret;
    
    switch(type) {
        case APE_PIPE_PUB_ID:
            h = ape->hashs.pipes.pub;
            break;
        case APE_PIPE_PRIV_ID:
            h = ape->hashs.pipes.priv;
            break;
    }
    
   /* do {
    
        
    } while
    */
    return ret;
}

ape_pipe *APE_pipe_new(ape_global *ape)
{
    ape_pipe *pipe = malloc(sizeof(*pipe));
    
    return pipe;
}
