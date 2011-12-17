//#define DEBUG 1
//#include "../deps/jsapi/src/jsapi.h"
#include <jsapi.h>
#include <APEapi.h>
#include <ape_events.h>
#include <glob.h>
#include <ape_sha1.h>
#include <ape_timers.h>

#include "ape_jsapi.h"

#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <fts.h>
#include <unistd.h>

D_APE_JS_NATIVE(ape_native_addListener);
D_APE_JS_NATIVE(ape_native_socket_listen);
D_APE_JS_NATIVE(ape_native_event_constructor);
D_APE_JS_NATIVE(ape_native_add_event_listener);
D_APE_JS_NATIVE(ape_native_echo);
D_APE_JS_NATIVE(ape_native_exit);
D_APE_JS_NATIVE(ape_native_dumpfile);
D_APE_JS_NATIVE(ape_native_readfile);
D_APE_JS_NATIVE(ape_native_socket_write);
D_APE_JS_NATIVE(ape_native_socket_sendfile);
D_APE_JS_NATIVE(ape_native_socket_close);
D_APE_JS_NATIVE(ape_native_glob);
D_APE_JS_NATIVE(ape_native_sha1_str);
D_APE_JS_NATIVE(ape_native_mkdir);
D_APE_JS_NATIVE(ape_native_rename);
D_APE_JS_NATIVE(ape_native_scandir);
D_APE_JS_NATIVE(ape_native_gc);
D_APE_JS_NATIVE(ape_native_file_exists);
D_APE_JS_NATIVE(ape_native_intern_string);
D_APE_JS_NATIVE(ape_native_dump_heap);
D_APE_JS_NATIVE(ape_native_set_timeout);
D_APE_JS_NATIVE(ape_native_set_interval);
D_APE_JS_NATIVE(ape_native_clear_timeout);

static void ape_jsapi_timer_wrapper(struct _ape_sm_timer *params, int *last);

static JSClass global_class = {
	"_GLOBAL", JSCLASS_GLOBAL_FLAGS | JSCLASS_IS_GLOBAL,
	    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
	    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass socket_class = {
	"Socket", JSCLASS_HAS_PRIVATE,
	    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
	    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass server_class = {
	"Server", JSCLASS_HAS_PRIVATE,
	    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
	    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass event_class = {
	"Event", JSCLASS_HAS_PRIVATE,
	    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
	    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec default_events[] = {
	JS_FS("addEventListener",   ape_native_addListener, 1, 0),
	JS_FS_END
};

static JSFunctionSpec socket_funcs[] = {
	JS_FS("listen",   ape_native_socket_listen, 2, 0),
	JS_FS("write", ape_native_socket_write, 1, 0),
	JS_FS("sendfile", ape_native_socket_sendfile, 1, 0),
	JS_FS("close", ape_native_socket_close, 0, 0),
	JS_FS_END
};

static JSFunctionSpec event_funcs[] = {
	JS_FS("addEventListener",  ape_native_add_event_listener, 2, 0),
	JS_FS_END
};

static JSFunctionSpec gbl_funcs[] = {
    JS_FS("exit",   ape_native_exit, 0, 0),
	JS_FS("echo",   ape_native_echo, 1, 0),
	JS_FS("dumpfile", ape_native_dumpfile, 2, 0),
	JS_FS("glob", ape_native_glob, 1, 0),
	JS_FS("readfile", ape_native_readfile, 1, 0),
	JS_FS("sha1", ape_native_sha1_str, 2, 0),
	JS_FS("mkdir", ape_native_mkdir, 1, 0),
	JS_FS("rename", ape_native_rename, 2, 0),
	JS_FS("file_exists", ape_native_file_exists, 1, 0),
	JS_FS("scandir", ape_native_scandir, 1, 0),
	JS_FS("JSGC", ape_native_gc, 1, 0),
	JS_FS("InternString", ape_native_intern_string, 1, 0),
	JS_FS("setTimeout", ape_native_set_timeout, 2, 0),
	JS_FS("setInterval", ape_native_set_interval, 2, 0),
	JS_FS("clearTimeout", ape_native_clear_timeout, 1, 0),
	JS_FS("clearInterval", ape_native_clear_timeout, 1, 0),
	#ifdef DEBUG
	JS_FS("dumpheap", ape_native_dump_heap, 0, 0),
	#endif
	JS_FS_END
};

APE_JS_NATIVE(ape_native_gc)
//{
	
	JS_GC(cx);
	return JS_TRUE;
}


APE_JS_NATIVE(ape_native_rename)
//{
	JSString *old, *new;
	char *cold, *cnew;
	
	if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vpn), "SS", &old, &new)) {
		return JS_TRUE;
	}
	
	cold = JS_EncodeString(cx, old);
	cnew = JS_EncodeString(cx, new);
	
	if (rename(cold, cnew) == -1) {
		printf("Err : %s\n", strerror(errno));
	}
	
	JS_free(cx, cold);
	JS_free(cx, cnew);
	
	return JS_TRUE;
}


APE_JS_NATIVE(ape_native_mkdir)
//{
	JSString *string;
	char *path;
	
    char opath[256];
    char *p;
    size_t len;
    
	if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vpn), "S", &string)) {
		return JS_TRUE;
	}
	
	path = JS_EncodeString(cx, string);
	
    strncpy(opath, path, sizeof(opath));
    len = strlen(opath);
    if(opath[len - 1] == '/')
            opath[len - 1] = '\0';
    for(p = opath; *p; p++)
            if(*p == '/') {
                    *p = '\0';
                    if(access(opath, F_OK))
                            mkdir(opath, S_IRWXO | S_IRWXU | S_IRWXO);
                    *p = '/';
            }
    if(access(opath, F_OK))         /* if path is not terminated with / */
            mkdir(opath, S_IRWXO | S_IRWXU | S_IRWXO);
            
    JS_free(cx, path);
    
   	return JS_TRUE;
}

APE_JS_NATIVE(ape_native_sha1_str)
//{
	JSString *string, *hmac = NULL;
	unsigned char digest[20];
	char *cstring, *chmac;
	char output[40];
	unsigned int i;
	
	if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vpn), "S/S", &string, &hmac)) {
		return JS_TRUE;
	}

	cstring = JS_EncodeString(cx, string);
	
	if (hmac == NULL) {
		sha1_csum((unsigned char *)cstring, JS_GetStringEncodingLength(cx, string), digest);
	} else {
		chmac = JS_EncodeString(cx, hmac);
		sha1_hmac((unsigned char *)chmac, JS_GetStringEncodingLength(cx, hmac), (unsigned char *)cstring, JS_GetStringEncodingLength(cx, string), digest);
		JS_free(cx, chmac);
	}
	
	for (i = 0; i < 20; i++) {
		sprintf(output + (i*2), "%.2x", digest[i]);
	}
	
	JS_SET_RVAL(cx, vpn, STRING_TO_JSVAL(JS_NewStringCopyN(cx, output, 40)));
	
	JS_free(cx, cstring);
	
	return JS_TRUE;	
}

APE_JS_NATIVE(ape_native_set_timeout)
//{
	struct _ape_sm_timer *params;
	struct _ticks_callback *timer;
	int ms, i;
	JSObject *obj = JS_THIS_OBJECT(cx, vpn);
	
	params = JS_malloc(cx, sizeof(*params));
	
	if (params == NULL) {
		return JS_FALSE;
	}
	
	params->cx = cx;
	params->global = obj;
	params->argc = argc-2;
	params->cleared = 0;
	params->timer = NULL;
	
	params->argv = (argc-2 ? JS_malloc(cx, sizeof(*params->argv) * argc-2) : NULL);
	
	if (!JS_ConvertValue(cx, JS_ARGV(cx, vpn)[0], JSTYPE_FUNCTION, &params->func)) {
		return JS_TRUE;
	}

	if (!JS_ConvertArguments(cx, 1, &JS_ARGV(cx, vpn)[1], "i", &ms)) {
		return JS_TRUE;
	}
	
	JS_AddValueRoot(cx, &params->func);
	
	for (i = 0; i < argc-2; i++) {
		params->argv[i] = JS_ARGV(cx, vpn)[i+2];
	}
	
	timer = add_timeout(ms, ape_jsapi_timer_wrapper, params, ape);
	timer->protect = 0;
	params->timer = timer;
	
	JS_SET_RVAL(cx, vpn, INT_TO_JSVAL(timer->identifier));
	
	return JS_TRUE;
}

APE_JS_NATIVE(ape_native_set_interval)
//{
	struct _ape_sm_timer *params;
	struct _ticks_callback *timer;
	int ms, i;
	JSObject *obj = JS_THIS_OBJECT(cx, vpn);
	
	params = JS_malloc(cx, sizeof(*params));
	
	if (params == NULL) {
		return JS_FALSE;
	}
	
	params->cx = cx;
	params->global = obj;
	params->argc = argc-2;
	params->cleared = 0;
	params->timer = NULL;
	
	params->argv = (argc-2 ? JS_malloc(cx, sizeof(*params->argv) * argc-2) : NULL);
	
	if (!JS_ConvertValue(cx, JS_ARGV(cx, vpn)[0], JSTYPE_FUNCTION, &params->func)) {
		return JS_TRUE;
	}

	if (!JS_ConvertArguments(cx, 1, &JS_ARGV(cx, vpn)[1], "i", &ms)) {
		return JS_TRUE;
	}
	
	JS_AddValueRoot(cx, &params->func);
	
	for (i = 0; i < argc-2; i++) {
		params->argv[i] = JS_ARGV(cx, vpn)[i+2];
	}
	
	timer = add_periodical(ms, 0, ape_jsapi_timer_wrapper, params, ape);
	timer->protect = 0;
	params->timer = timer;
	
	JS_SET_RVAL(cx, vpn, INT_TO_JSVAL(timer->identifier));
	
	return JS_TRUE;	
}

APE_JS_NATIVE(ape_native_clear_timeout)
//{
	unsigned int identifier;
	struct _ape_sm_timer *params;
	struct _ticks_callback *timer;
	
	if (!JS_ConvertArguments(cx, 1, JS_ARGV(cx, vpn), "i", &identifier)) {
		return JS_TRUE;
	}
	
	if ((timer = get_timer_identifier(identifier, ape)) != NULL && !timer->protect) {
		params = timer->params;
		params->cleared = 1;
	}

	return JS_TRUE;
}


APE_JS_NATIVE(ape_native_addListener)
//{

	return JS_TRUE;
}

APE_JS_NATIVE(ape_native_exit)
//{
    exit(1);
	return JS_TRUE;
}

APE_JS_NATIVE(ape_native_echo)
//{
    char *cstring;
    JSString *string;
    
    if (!JS_ConvertArguments(cx, 1, JS_ARGV(cx, vpn), "S", &string)) {
        return JS_TRUE;
    }
    
    JS_SET_RVAL(cx, vpn, JSVAL_NULL);

    cstring = JS_EncodeString(cx, string);

    fwrite(cstring, sizeof(char),
        JS_GetStringEncodingLength(cx, string), stdout);

    JS_free(cx, cstring);

    return JS_TRUE;
}


APE_JS_NATIVE(ape_native_scandir)
//{
	FTS *tree;
    JSString *dir;
    char *cdir;
    char *paths[2];
    FTSENT *node;
    JSObject *ret;
    int i = 0;

    if (!JS_ConvertArguments(cx, 1, JS_ARGV(cx, vpn), "S", &dir)) {
        return JS_TRUE;
    }
    
    cdir = JS_EncodeString(cx, dir);
    
    paths[0] = cdir;
    paths[1] = 0;
    
    tree = fts_open(paths, FTS_NOCHDIR, 0);
    
    printf("Dir opened\n");
    
    if (!tree) {
		return JS_TRUE;
    }
	
	ret = JS_NewArrayObject(cx, 0, NULL);

    while ((node = fts_read(tree))) {
        if (node->fts_level > 0 && node->fts_name[0] == '.')
            fts_set(tree, node, FTS_SKIP);
        else if (node->fts_info & FTS_F) {
        	JSString *file = JS_NewStringCopyZ(cx, node->fts_accpath);
			jsval val = STRING_TO_JSVAL(file);
			JS_SetElement(cx, ret, i++, &val);
        }
    }
    
    JS_SET_RVAL(cx, vpn, OBJECT_TO_JSVAL(ret));
    
    fts_close(tree);
    JS_free(cx, cdir);
    
	return JS_TRUE;
	
}


APE_JS_NATIVE(ape_native_glob)
//{
    JSString *dir;
    char *cdir;
    glob_t globbuf;
    int i;
    JSObject *ret;

    if (!JS_ConvertArguments(cx, 1, JS_ARGV(cx, vpn), "S", &dir)) {
        return JS_TRUE;
    }
    
    JS_SET_RVAL(cx, vpn, JSVAL_NULL);

    cdir = JS_EncodeString(cx, dir);
    
    if (glob(cdir, 0, NULL, &globbuf) == 0) {
        
        ret = JS_NewArrayObject(cx, globbuf.gl_pathc, NULL);
        
        if (ret == NULL) {
            globfree(&globbuf);
            JS_free(cx, cdir);
            printf("OOM on glob\n");
            return JS_TRUE;
        }
        for (i = 0; i < globbuf.gl_pathc; i++) {
            JSString *file = JS_NewStringCopyN(cx, globbuf.gl_pathv[i], strlen(globbuf.gl_pathv[i]));
            jsval val = STRING_TO_JSVAL(file);
            
            JS_SetElement(cx, ret, i, &val);
            
        }
        globfree(&globbuf);
        
        JS_SET_RVAL(cx, vpn, OBJECT_TO_JSVAL(ret));
    }
    
    JS_free(cx, cdir);
    
    return JS_TRUE;
}

#ifdef DEBUG
APE_JS_NATIVE(ape_native_dump_heap)
//{
	FILE *fp;
	
	fp = fopen("/tmp/JSDUMP", "w+");
	
	JS_DumpHeap(cx, fp, NULL, 0, NULL, 1024, NULL);
	
	fclose(fp);
	
	return JS_TRUE;
}
#endif

APE_JS_NATIVE(ape_native_intern_string)
//{
    JSString *str;
    
    if (!JS_ConvertArguments(cx, 1, JS_ARGV(cx, vpn), "S", &str)) {
        return JS_TRUE;
    }
    
    JS_InternJSString(cx, str);
    
	return JS_TRUE;
}

APE_JS_NATIVE(ape_native_file_exists)
//{
    char *cfile;
    JSString *file;
    
    if (!JS_ConvertArguments(cx, 1, JS_ARGV(cx, vpn), "S", &file)) {
    	printf("Cant convert arg\n");
        return JS_TRUE;
    }
    
    JS_SET_RVAL(cx, vpn, JSVAL_FALSE);
    
    cfile = JS_EncodeString(cx, file);
    
    if (access(cfile, F_OK) == 0) {
    	JS_SET_RVAL(cx, vpn, JSVAL_TRUE);
    }
    
    JS_free(cx, cfile);
    
    return JS_TRUE;
}

APE_JS_NATIVE(ape_native_readfile)
//{
    char *cfile;
    JSString *file, *ret;
    FILE *fd;
    char *data = NULL;
    size_t size = 0;

    if (!JS_ConvertArguments(cx, 1, JS_ARGV(cx, vpn), "S", &file)) {
    	printf("Cant convert arg\n");
        return JS_TRUE;
    }
    
    JS_SET_RVAL(cx, vpn, JSVAL_NULL);

    cfile = JS_EncodeString(cx, file);
    
    if (cfile == NULL) {
    	printf("CANNOT CREATE STRING\n");
    	
    	return JS_TRUE;
    }
    
    fd = fopen(cfile, "r");
    
    if (fd == NULL) {
        JS_free(cx, cfile);
        printf("Cannot open file\n");
        return JS_TRUE;   
    }
    
    while(!feof(fd)) {
            
        data = (data == NULL ? JS_malloc(cx, sizeof(char) * 1024) : 
                                JS_realloc(cx, data, sizeof(char) * (1024+size)));
        
        if (data == NULL) {
            JS_free(cx, cfile);
            fclose(fd);
            
            printf("OOM in readfile\n");
            
            return JS_TRUE;
        }
        size += fread(data+size, sizeof(char), 1024, fd);
    }
    
    ret = JS_NewStringCopyN(cx, data, size);
    if (ret == NULL) {
        printf("OOM in readfile (copy to engine)\n");
    } else {
        JS_SET_RVAL(cx, vpn, STRING_TO_JSVAL(ret));
    }
    JS_free(cx, data);
    JS_free(cx, cfile);
    fclose(fd);

    return JS_TRUE;   
}

APE_JS_NATIVE(ape_native_dumpfile)
//{
    char *cstring, *cfile;
    JSString *string, *file;
    FILE *fd;

    if (!JS_ConvertArguments(cx, 2, JS_ARGV(cx, vpn), "SS", &file, &string)) {
        return JS_TRUE;
    }
    
    JS_SET_RVAL(cx, vpn, JSVAL_NULL);

    cstring = JS_EncodeString(cx, string);
    cfile = JS_EncodeString(cx, file);
    
    fd = fopen(cfile, "w+");
    
    if (fd == NULL) {
        JS_free(cx, cstring);
        JS_free(cx, cfile);     
        
        return JS_TRUE;   
    }
    
    //printf("Writing to file %s (%d)\n", cfile, JS_GetStringEncodingLength(cx, string));
    fwrite(cstring, sizeof(char),
        JS_GetStringEncodingLength(cx, string), fd);

    fclose(fd);
    
    /* TODO: 33 = www-data !!! (use getpwname) */
    chown(cfile, 33, 33);

    JS_free(cx, cstring);
    JS_free(cx, cfile);

    return JS_TRUE;
}


APE_JS_NATIVE(ape_native_event_constructor)
//{
    JSObject *obj = JS_NewObjectForConstructor(cx, vpn);
    ape_js_private *privates = JS_malloc(cx, sizeof(*privates));
    
    privates->slots[0] = ape_array_new(8);
    JS_SetPrivate(cx, obj, (void *)privates);
    
    *vpn = OBJECT_TO_JSVAL(obj);

    return JS_TRUE;
}

void ape_js_trigger_event(JSContext *cx, JSObject *obj, const char *name, uintN argc, jsval *argv)
{
    ape_array_t *callbacks;
    ape_js_events *events;
    ape_js_private *privates;
    jsval rval;
    
    #if 0
    if ((callbacks = JS_GetInstancePrivate(cx, obj, &event_class,
                        NULL)) == NULL) {
        return;
    }
    #endif
    if ((privates = JS_GetPrivate(cx, obj)) == NULL) {
        return;
    }

    callbacks = privates->slots[0];
        
    events = ape_array_lookup_data(callbacks, name, strlen(name));
    
    while (events != NULL) {
        JS_CallFunctionValue(cx, obj, events->func, argc, argv, &rval);
        
        events = events->next;
    }
}

APE_JS_NATIVE(ape_native_add_event_listener)
//{
    JSString *event;
    JSObject *obj = JS_THIS_OBJECT(cx, vpn);
    ape_array_t *callbacks;
    jsval cbfunc;
    ape_js_events *events = NULL, *nevent = NULL;
    char *cstring;
    ape_array_item_t *array_i;
    ape_js_private *privates;
    
    #if 0
    if ((callbacks = JS_GetInstancePrivate(cx, obj, &event_class,
                        JS_ARGV(cx, vpn))) == NULL) {
        return JS_TRUE;
    }
    #endif
    if ((privates = JS_GetPrivate(cx, obj)) == NULL) {
    
        /* TODO: Create the private */
        return JS_TRUE;
    }
    
	if (!JS_ConvertArguments(cx, 1, JS_ARGV(cx, vpn), "S", &event)) {
		return JS_TRUE;
	}
	
	if (!JS_ConvertValue(cx, JS_ARGV(cx, vpn)[1], JSTYPE_FUNCTION, &cbfunc)) {
		return JS_TRUE;
	}

	cstring = JS_EncodeString(cx, event);
    nevent = JS_malloc(cx, sizeof(*nevent));
    callbacks = privates->slots[0];
    	
	array_i = ape_array_lookup_item(callbacks, cstring, 
	                                JS_GetStringEncodingLength(cx, event));

    if (array_i != NULL) {
        events = array_i->pool.ptr.data;
    } else {
        array_i = ape_array_add_ptrn(callbacks, cstring,
                    JS_GetStringEncodingLength(cx, event), nevent);
    }
    
    nevent->next = events;
    nevent->func = cbfunc;
    
    JS_AddValueRoot(cx, &nevent->func);
    
    array_i->pool.ptr.data = nevent;
    
    JS_free(cx, cstring);
    JS_SET_RVAL(cx, vpn, JSVAL_TRUE);
 	
	//printf("done\n");   
    //ape_js_trigger_event(cx, obj, "a");

	return JS_TRUE;
}

APE_JS_NATIVE(ape_native_socket_sendfile)
//{
    JSObject *obj = JS_THIS_OBJECT(cx, vpn);
    ape_js_private *privates;
    ape_socket *sock;
    
	JSString *string;
	char *cstring;
	
    if ((privates = JS_GetPrivate(cx, obj)) == NULL) {
        return JS_TRUE;
    }

    sock = privates->slots[1];
    
	if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vpn), "S", &string)) {
		return JS_TRUE;
	}
	
    cstring = JS_EncodeString(cx, string);
    
    sock = privates->slots[1];
    
    JS_SET_RVAL(cx, vpn, JSVAL_TRUE);
    
    if (APE_sendfile(sock, cstring) == 0) {
        JS_SET_RVAL(cx, vpn, JSVAL_FALSE);
    }
    
    JS_free(cx, cstring);
    
    return JS_TRUE;
}

APE_JS_NATIVE(ape_native_socket_close)
//{
    JSObject *obj = JS_THIS_OBJECT(cx, vpn);
    ape_js_private *privates;
    ape_socket *sock;

    if ((privates = JS_GetPrivate(cx, obj)) == NULL) {
        return JS_TRUE;
    }

    sock = privates->slots[1];
    
    APE_socket_shutdown(sock);
    
    return JS_TRUE;
}

APE_JS_NATIVE(ape_native_socket_write)
//{
    JSObject *obj = JS_THIS_OBJECT(cx, vpn);
    ape_js_private *privates;
    ape_socket *sock;
    
	JSString *string;
	char *cstring;
	size_t lstring;
    
    if ((privates = JS_GetPrivate(cx, obj)) == NULL) {
        return JS_TRUE;
    }
    
	if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vpn), "S", &string)) {
		return JS_TRUE;
	}
	
	lstring = JS_GetStringEncodingLength(cx, string);
	cstring = malloc(lstring);
	lstring = JS_EncodeStringToBuffer(string, cstring, lstring);

    sock = privates->slots[1];
    
    if (APE_socket_write(sock, cstring, lstring) == 0) {
        free(cstring);
    } else {
		printf("FAILED TO WRITE\n");
	}
    
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

static void ape_js_socket_connected(ape_socket *sock, ape_global *ape)
{
    JSContext *cx = ape_get_property(ape->extend, "jsapi", 5);
    JSObject *obj = sock->ctx;

    ape_js_trigger_event(cx, obj, "connect", 0, NULL);
}

void ape_js_socket_read(ape_socket *socket_client, ape_global *ape)
{
    JSContext *cx = ape_get_property(ape->extend, "jsapi", 5);
    JSObject *obj = socket_client->ctx;
    JSString *data;
    jsval ret;
    
    data = JS_NewStringCopyN(cx, socket_client->data_in.data, socket_client->data_in.used);
    
    ret = STRING_TO_JSVAL(data);
    
    //printf("Reading... %s\n", socket_client->data_in.data);
    
    ape_js_trigger_event(cx, obj, "read", 1, &ret);
}

void ape_js_socket_disconnect(ape_socket *socket_client, ape_global *ape)
{
    JSContext *cx = ape_get_property(ape->extend, "jsapi", 5);
    JSObject *obj = socket_client->ctx;
    
    ape_js_trigger_event(cx, obj, "disconnect", 0, NULL);
}

APE_JS_NATIVE(ape_native_socket_constructor)
//{
	JSObject *obj = JS_NewObjectForConstructor(cx, vpn);
	JSString *ip;
	char *cip;
	jsval vp;
	ape_socket *socket;
	int port;
	ape_js_private *privates;
	
	if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vpn), "Si", &ip, &port)) {
		return JS_TRUE;
	}
	
	privates = JS_malloc(cx, sizeof(*privates));
	
	cip = JS_EncodeString(cx, ip);
	
	socket 		    = APE_socket_new(APE_SOCKET_PT_TCP, 0);
	
	if (socket == NULL) {
		printf("=============\n\nCannot allocate socket\n===================\n");
		return JS_TRUE;
	}
	
	socket->ctx 	= obj; /* obj can be gc collected */
	
	socket->callbacks.on_connected = ape_js_socket_connected;
	socket->callbacks.on_disconnect = ape_js_socket_disconnect;
	socket->callbacks.on_read = ape_js_socket_read;
	
    if (APE_socket_connect(socket, port, cip, ape) < 0) {
        printf("Failed to connect\n");
    }
    
    /* TODO: free */
    privates->slots[0] = ape_array_new(8);
    privates->slots[1] = socket;
    
    JS_SetPrivate(cx, obj, (void *)privates);

	*vpn = OBJECT_TO_JSVAL(obj);

	return JS_TRUE;
}

static JSObject *ape_socket_to_jsobj(JSContext *cx, ape_socket *socket, 
            ape_global *ape)
{
    JSObject *obj;
    ape_js_private *privates;
    
    if (socket->ctx != NULL) {
        return (JSObject *)socket->ctx;
    }
    
    /* TODO: free */
    privates = JS_malloc(cx, sizeof(*privates));
    
    obj = JS_NewObjectWithGivenProto(cx, &socket_class, 
            ape_get_property(ape->extend, "jsevents", 8), NULL);
            
    JS_DefineFunctions(cx, obj, socket_funcs);
            
    privates->slots[0] = ape_array_new(8);
    privates->slots[1] = socket;
    
    JS_SetPrivate(cx, obj, (void *)privates);
    socket->ctx = obj;
    
    return obj;
}

static void ape_jsapi_timer_wrapper(struct _ape_sm_timer *params, int *last)
{
	jsval rval;

	if (!params->cleared) {
		JS_CallFunctionValue(params->cx, params->global, params->func, 
		                        params->argc, params->argv, &rval);
	}
	
	if (params->cleared && !*last) { /* JS_CallFunctionValue can set params->Cleared to true */
        *last = 1;
	}
	
	if (*last) {
		JS_RemoveValueRoot(params->cx, &params->func);
		
		if (params->argv != NULL) {
			free(params->argv);
		}
		free(params);
	}

}


static void ape_jsapi_report_error(JSContext *cx, const char *message,
    JSErrorReport *report)
{
    fprintf(stderr, "%s:%u:%s\n",
            report->filename ? report->filename : "<no filename>",
            (unsigned int) report->lineno,
            message);
}

static void JS_InitAPEClasses(JSContext *cx, ape_global *ape)
{
	JSObject *gbl = JS_GetGlobalObject(cx);
	JSObject *sockserver;
	JSObject *events;
	JSObject *server;
	ape_js_private *privates;
	
/*
	JS_DefineFunctions(cx, obj, default_events);
	JS_DefineFunctions(cx, obj, socket_funcs);
*/
    JS_DefineFunctions(cx, gbl, gbl_funcs);
    events = JS_InitClass(cx, gbl, NULL, &event_class, ape_native_event_constructor, 0, NULL, event_funcs, NULL, NULL);
    
    ape_add_property(ape->extend, "jsevents", events);

	/*TODO: Make all APE function prototype of an evented object*/
	JS_InitClass(cx, gbl, events, &socket_class, ape_native_socket_constructor, 2, NULL, socket_funcs, NULL, NULL);
	
	server = JS_DefineObject(cx, gbl, "Server", &server_class, events, 0);
	
	ape_add_property(ape->extend, "jsserver", server);
	
	privates = JS_malloc(cx, sizeof(*privates));
	
    privates->slots[0] = ape_array_new(8);
    
    JS_SetPrivate(cx, server, (void *)privates);
}

static JSContext *ape_jsapi_createcontext(JSRuntime *rt, ape_global *ape)
{
	JSContext *cx;
	JSObject *gbl;

	if ((cx = JS_NewContext(rt, 8192)) == NULL) {
		return NULL;
	}
	
	#if 0
	JS_SetGCZeal(cx, 2);
    #endif
    
	JS_SetOptions(cx, JSOPTION_VAROBJFIX | /*JSOPTION_JIT | */JSOPTION_METHODJIT | JSOPTION_METHODJIT_ALWAYS | JSOPTION_TYPE_INFERENCE);
	JS_SetVersion(cx, JSVERSION_LATEST);

	JS_SetErrorReporter(cx, ape_jsapi_report_error);

	gbl = JS_NewCompartmentAndGlobalObject(cx, &global_class, NULL);

	JS_InitStandardClasses(cx, gbl);
	JS_InitAPEClasses(cx, ape);

	JS_SetContextPrivate(cx, ape);

	ape_add_property(ape->extend, "jsapi", cx);

	return cx;
}

static int ape_module_jsapi_init(ape_global *ape)
{
	JSRuntime *rt;
	glob_t globbuf;

	int i;

	JS_SetCStringsAreUTF8();
	
	if ((rt = JS_NewRuntime(1024L * 1024L * 1024L)) == NULL) {
		return -1;
	}

	printf("[SSJS] JSAPI runtime initialized\n");

	if (ape_jsapi_createcontext(rt, ape) == NULL) {
		return -1;
	}

	return 0;
}

static int ape_module_jsapi_destroy(ape_global *ape)
{
	JSRuntime *rt;
	JSContext *cx;
	
	cx = ape_get_property(ape->extend, "jsapi", 5);
	rt = JS_GetRuntime(cx);
	
    JS_DestroyContext(cx);
	JS_DestroyRuntime(rt);
	JS_ShutDown();
	
	return 0;
}

static int ape_module_jsapi_loaded(ape_global *ape)
{
    JSContext *cx = ape_get_property(ape->extend, "jsapi", 5);
    jsval result;
    JSScript *code;

    if (cx == NULL) {
        return -1;
    }

    if ((code = JS_CompileFile(cx, JS_GetGlobalObject(cx),
            "../../scripts/main.js")) == NULL) {
        return -1;
    }

	if (JS_ExecuteScript(cx, JS_GetGlobalObject(cx), code, &result)
	        == JS_FALSE) {
		return -1;
	}

	return 0;
}

static int ape_module_jsapi_request(ape_client *client, ape_global *ape)
{
    const char *chroot = ((ape_server *)client->server->_ctx)->chroot;
    JSContext *cx = ape_get_property(ape->extend, "jsapi", 5);
    JSObject *server = ape_get_property(ape->extend, "jsserver", 8);
    JSObject *socket = ape_socket_to_jsobj(cx, client->socket, ape);
    jsval args[2];
    JSString *jspath, *jschroot, *jsfullpath;
    
    jschroot = JS_NewStringCopyZ(cx, ((ape_server *)client->server->_ctx)->chroot);
    jspath = JS_NewStringCopyN(cx, &client->http.path->data[1], client->http.path->used-2);
    jsfullpath = JS_ConcatStrings(cx, jschroot, jspath);
    
    args[0] = OBJECT_TO_JSVAL(socket);
    args[1] = STRING_TO_JSVAL(jspath);
    
    ape_js_trigger_event(cx, server, "request", 2, args);
    
    
    /* TODO: create Server global object with events request and trigger it here */

}

ape_module_t ape_jsapi_module = {
	"APE JSAPI",
	ape_module_jsapi_init,
	ape_module_jsapi_loaded,
	ape_module_jsapi_request,
	ape_module_jsapi_destroy,
	/*ape_module_jsapi_finish*/
};

