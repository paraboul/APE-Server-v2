#ifndef __APE_TRANSPORTS_H_
#define __APE_TRANSPORTS_H_

#include "common.h"

#define APE_STATIC_URI "/static/"

typedef enum {
    APE_TRANSPORT_LP,  /* Long polling */
    APE_TRANSPORT_WS,  /* WebSocket */
    APE_TRANSPORT_FT,  /* File transfert */
    APE_TRANSPORT_NU   /* Unknown transport */
} ape_transport_t;



#define APE_TRANSPORT_QS_ISJSON(t) (t != APE_TRANSPORT_FT && t != APE_TRANSPORT_NU)


#endif

// vim: ts=4 sts=4 sw=4 et

