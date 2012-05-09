#ifndef __APE_JSAPI_H
#define __APE_JSAPI_H

//#define DEBUG 1

//#include "../deps/jsapi/src/jsapi.h"

#include <jsapi.h>

#define D_APE_JS_NATIVE(func_name) static JSBool func_name(JSContext *cx, uintN argc, jsval *vpn)

#define APE_JS_NATIVE(func_name) \
	static JSBool func_name(JSContext *cx, uintN argc, jsval *vpn) \
	{\
		ape_global *ape; \
		ape = JS_GetContextPrivate(cx);
		
typedef struct _ape_js_events {
    jsval func;
    struct _ape_js_events *next;
} ape_js_events;

typedef struct {
    void *slots[8];
} ape_js_private;

struct _ape_sm_timer
{
	JSContext *cx;
	JSObject *global;
	jsval func;
	
	uintN argc;
	jsval *argv;
	
	int cleared;
	struct _ticks_callback *timer;
};

void ape_js_trigger_event(JSContext *cx, JSObject *obj, const char *name, uintN argc, jsval *argv);

#endif

