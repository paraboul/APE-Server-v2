#ifndef __APE_MESSAGE_H_
#define __APE_MESSAGE_H_

#include "ape_buffer.h"

typedef struct _ape_push_message_
{
    buffer msg;
    int ret_count;
} ape_push_msg_t;

#endif
