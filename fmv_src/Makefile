include ../Makefile.inc
CFLAGS += -I../supp_src

all: libfmv.a

gen_fmv.h: message_template.msg parse-ng.lua
	$(LUA) parse-ng.lua header message_template.msg >gen_fmv.h


gen_fmv.c: message_template.msg parse-ng.lua
	$(LUA) parse-ng.lua code message_template.msg >gen_fmv.c

sta_fmv.o: sta_fmv.c sta_fmv.h
	$(CC) -c $(CFLAGS) sta_fmv.c

lua_fmv.o: lua_fmv.c lua_fmv.h
	$(CC) -c $(CFLAGS) lua_fmv.c

gen_fmv.o: gen_fmv.h gen_fmv.c sta_fmv.h
	$(CC) -c $(CFLAGS) gen_fmv.c

libfmv.a: gen_fmv.o sta_fmv.o lua_fmv.o
	$(AR) $@ $?
	$(RANLIB) $@

clean:
	rm -f gen_fmv.[ch]
	rm -f *.o
	rm -f libfmv.a

