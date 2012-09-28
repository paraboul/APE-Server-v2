CC=gcc
CFLAGS=-g -I../core/ -I../../deps/ -I/usr/include/ -Wno-deprecated-declarations -I/Users/anthonycatel/Nativestudio/mozilla/js/src/dist/include -I/Users/anthonycatel/Nativestudio/mozilla/js/src/
LDFLAGS=../../deps/c-ares/.libs/libcares.a /Users/anthonycatel/Nativestudio/mozilla/js/src/libjs_static.a ../../deps/confuse-2.7/src/.libs/libconfuse.a -lm -lstdc++ -lssl -lcrypto

NO_COLOR=\x1b[0m
OK_COLOR=\x1b[32;01m
ERROR_COLOR=\x1b[31;01m
WARN_COLOR=\x1b[33;01m

SOURCES= JSON_parser.c 			\
		ape_array.c 			\
		ape_base64.c 			\
		ape_buffer.c 			\
		ape_channels.c 			\
		ape_cmd.c 				\
		ape_config.c 			\
		ape_dns.c 				\
		ape_entry.c 			\
		ape_event_devpoll.c 	\
		ape_event_epoll.c 		\
		ape_event_kqueue.c 		\
		ape_event_select.c 		\
		ape_events.c 			\
		ape_events_loop.c 		\
		ape_extend.c 			\
		ape_hash.c 				\
		ape_http_parser.c 		\
		ape_json.c 				\
		ape_log.c 				\
		ape_message.c 			\
		ape_pipe.c 				\
		ape_pool.c 				\
		ape_server.c 			\
		ape_sha1.c 				\
		ape_socket.c 			\
		ape_ssl.c 				\
		ape_string.c 			\
		ape_timers.c 			\
		ape_transports.c 		\
		ape_user.c 				\
		ape_websocket.c 		\
		../modules/ape_jsapi.c  \
		../modules/ape_modules.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=../../bin/aped

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "$(ERROR_COLOR)Linking$(NO_COLOR)" to "$(OK_COLOR)"$(EXECUTABLE)"$(NO_COLOR)"...
.c.o:
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "[$(OK_COLOR)CC$(NO_COLOR)]" "$(WARN_COLOR)"$<"$(NO_COLOR)"
	
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)