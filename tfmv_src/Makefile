include ../Makefile.inc
CFLAGS += -I../supp_src

all: libtfmv.so

trans_fmv.h: message_template.msg parse-ng.lua
	lua parse-ng.lua header message_template.msg >trans_fmv.h


trans_fmv.c: message_template.msg parse-ng.lua
	lua parse-ng.lua code message_template.msg >trans_fmv.c

template.o: template.c template.h
	$(CC) -c $(CFLAGS) template.c

trans_fmv.o: trans_fmv.h trans_fmv.c template.h
	$(CC) -c $(CFLAGS) trans_fmv.c

libtfmv.so: trans_fmv.o template.o
	$(CC) -shared -Wl,-soname,libfmv.so -o $@ $^
	cp $@ ../lib/

clean:
	rm -f trans_fmv.[ch]
	rm -f *.o
	rm -f libtfmv.so

