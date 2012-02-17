#include "common.h"
#include "ape_pipe.h"
#include "ape_hash.h"
#include "ape_base64.h"

static void ape_pipe_gen_id(ape_pipe *pipe, ape_pipe_id_t type,
                ape_global *ape)
{
    ape_htable_t *h;
    char *str;
    uint64_t *bin;
    int i = 0;
    
    switch(type) {
        case APE_PIPE_PUB_ID:
            h = ape->hashs.pipes.pub;
            str = pipe->id.pub.str;
            bin = &pipe->id.pub.bin;
            break;
        case APE_PIPE_PRIV_ID:
            h = ape->hashs.pipes.priv;
            str = pipe->id.priv.str;
            bin = &pipe->id.priv.bin;
            break;
    }
    
    do {
        *bin = ape_rand_64_base64_b(str);
    } while(hashtbl_seek64(h, *bin) != NULL);
    
    hashtbl_append64(h, *bin, pipe);
    
}

ape_pipe *APE_pipe_new(ape_global *ape)
{
    ape_pipe *pipe = malloc(sizeof(*pipe));
    
    ape_pipe_gen_id(pipe, APE_PIPE_PRIV_ID, ape);
    ape_pipe_gen_id(pipe, APE_PIPE_PUB_ID,  ape);
    
    pipe->link.ctx = NULL;
    
    return pipe;
}

