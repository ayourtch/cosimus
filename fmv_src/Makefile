include ../Makefile.inc
CFLAGS += -I../supp_src

all: libfmv.so

gen_fmv.h: message_template.msg parse-ng.lua
	$(LUA) parse-ng.lua header message_template.msg >gen_fmv.h


gen_fmv.c: message_template.msg parse-ng.lua
	$(LUA) parse-ng.lua code message_template.msg >gen_fmv.c

sta_fmv.o: sta_fmv.c sta_fmv.h
	$(CC) -c $(CFLAGS) sta_fmv.c

gen_fmv.o: gen_fmv.h gen_fmv.c sta_fmv.h
	$(CC) -c $(CFLAGS) gen_fmv.c

libfmv.so: gen_fmv.o sta_fmv.o
	$(CC) -shared -Wl,-soname,libfmv.so -o $@ $^
	cp $@ ../lib/

clean:
	rm -f gen_fmv.[ch]
	rm -f *.o
	rm -f libfmv.so

