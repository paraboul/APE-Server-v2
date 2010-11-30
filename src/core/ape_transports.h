#ifndef __APE_TRANSPORTS_H_
#define __APE_TRANSPORTS_H_

#define APE_STATIC_URI "/static/"

typedef enum {
	APE_TRANSPORT_LP,  /* Long polling */
	APE_TRANSPORT_WS,  /* WebSocket */
	APE_TRANSPORT_FT,  /* File transfert */
	APE_TRANSPORT_NU   /* Unknown transport */
} ape_transport_t;


static struct _ape_transports_s {
	ape_transport_t type;
	size_t len;
	char path[12];
} ape_transports_s[] = {
	{APE_TRANSPORT_LP, 3, "/1/"},
	{APE_TRANSPORT_WS, 3, "/2/"},
	{APE_TRANSPORT_FT, 8, APE_STATIC_URI},
	{APE_TRANSPORT_NU, 0, ""}
};


#define APE_TRANSPORT_QS_ISJSON(t) (t != APE_TRANSPORT_FT && t != APE_TRANSPORT_NU)


#endif
