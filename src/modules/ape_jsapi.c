#include "../deps/jsapi/src/jsapi.h"

#include <APEapi.h>
#include <events.h>
#include <glob.h>

#define D_APE_JS_NATIVE(func_name) static JSBool func_name(JSContext *cx, uintN argc, jsval *vpn)

#define APE_JS_NATIVE(func_name) \
	static JSBool func_name(JSContext *cx, uintN argc, jsval *vpn) \
	{\
		ape_global *ape; \
		ape = JS_GetContextPrivate(cx);


D_APE_JS_NATIVE(ape_native_addListener);		
D_APE_JS_NATIVE(ape_native_socket_listen);

static JSClass global_class = {
	"_GLOBAL", JSCLASS_GLOBAL_FLAGS,
	    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass socket_class = {
	"Socket", JSCLASS_HAS_PRIVATE,
	    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec default_events[] = {
	JS_FS("addEventListener",   ape_native_addListener, 1, 0),
	JS_FS_END
};

static JSFunctionSpec socket_funcs[] = {
	JS_FS("listen",   ape_native_socket_listen, 2, 0),
	JS_FS_END
};



APE_JS_NATIVE(ape_native_addListener)
//{
	
	return JS_TRUE;
}

APE_JS_NATIVE(ape_native_socket_listen)
//{
	JSObject *obj = JS_THIS_OBJECT(cx, vpn);
	ape_socket *socket = JS_GetPrivate(cx, obj);
	int port;
	
	JS_SET_RVAL(cx, vpn, JSVAL_FALSE);

	if (!JS_ConvertArguments(cx, 1, JS_ARGV(cx, vpn), "i", &port)) {
		return JS_TRUE;
	}
	
	APE_socket_listen(socket, (uint16_t)port, "127.0.0.1", ape);
	
	return JS_TRUE;
}

APE_JS_NATIVE(ape_native_socket_constructor)
//{
	JSObject *obj = JS_NewObjectForConstructor(cx, vpn);
	jsval vp;
	ape_socket *socket;

	
	socket 		= APE_socket_new(APE_SOCKET_PT_TCP, 0);
	socket->ctx 	= obj; /* obj can be gc collected */
	
	JS_SetPrivate(cx, obj, socket);
	
	*vpn = OBJECT_TO_JSVAL(obj);
		
	return JS_TRUE;
}

static void ape_jsapi_report_error(JSContext *cx, const char *message, JSErrorReport *report)
{
    fprintf(stderr, "%s:%u:%s\n",
            report->filename ? report->filename : "<no filename>",
            (unsigned int) report->lineno,
            message);
}

static void JS_InitAPEClasses(JSContext *cx)
{
	JSObject *gbl = JS_GetGlobalObject(cx);
	JSObject *sockserver;
/*

	JS_DefineFunctions(cx, obj, default_events);
	JS_DefineFunctions(cx, obj, socket_funcs);
*/	

	/*TODO: Make all APE function prototype of an evented object*/
	JS_InitClass(cx, gbl, NULL, &socket_class, ape_native_socket_constructor, 0, NULL, NULL, NULL, NULL);
	
}

static JSContext *ape_jsapi_createcontext(JSRuntime *rt, ape_global *ape)
{
	JSContext *cx;
	JSObject *gbl;
	
	if ((cx = JS_NewContext(rt, 8192)) == NULL) {
		return NULL;
	}
	
	JS_SetOptions(cx, JSOPTION_VAROBJFIX | JSOPTION_JIT | JSOPTION_METHODJIT);
	JS_SetVersion(cx, JSVERSION_LATEST);
	
	JS_SetErrorReporter(cx, ape_jsapi_report_error);
	
	gbl = JS_NewGlobalObject(cx, &global_class);
	JS_InitStandardClasses(cx, gbl);
	JS_InitAPEClasses(cx);
	
	JS_SetContextPrivate(cx, ape);
	
	return cx;
}

static int ape_module_jsapi_init(ape_global *ape)
{
	JSRuntime *rt;
	glob_t globbuf;
	
	int i;
	
	if ((rt = JS_NewRuntime(8L * 1024L * 1024L)) == NULL) {
		return -1;
	}
	
	printf("JSAPI initialized\n");
	
	glob("../../scripts/main.js", 0, NULL, &globbuf);
	
	for (i = 0; i < globbuf.gl_pathc; i++) {
		JSContext *cx;	
		JSScript *code;
		jsval result;

		
		if ((cx = ape_jsapi_createcontext(rt, ape)) == NULL) {
			return -1;
		}
		
		code = JS_CompileFile(cx, JS_GetGlobalObject(cx), globbuf.gl_pathv[i]);
		
		if (JS_ExecuteScript(cx, JS_GetGlobalObject(cx), code, &result) == JS_FALSE) {
			return -1;
		}
	}
	
	return 0;
}

ape_module_t ape_jsapi_module = {
	"APE JSAPI",
	ape_module_jsapi_init,
	/*ape_module_jsapi_finish*/
};

