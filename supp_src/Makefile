include ../Makefile.inc
LIBS=-lssl
SRCS= lib_dbuf.c lib_debug.c lib_hash.c lib_hash_func.c lib_lists.c lib_os.c lib_poll.c lib_sock.c
INCLUDES= lib_dbuf.h lib_debug.h lib_hash.h lib_hash_func.h lib_lists.h lib_os.h lib_poll.h lib_sock.h lib_sock_intern.h
OBJS= $(subst .c,.o,$(SRCS))

DEPEND=makedepend

all: supplib

supplib: depend libsupp.so

libsupp.so: $(OBJS)
	 $(CC) -shared -Wl,-soname,libsupp.so -o $@ $^ $(LIBS)

clean:
	rm -f *.o
	rm -f libsupp.so
	rm -f Makefile.depend

depend: Makefile.depend
	
Makefile.depend: $(SRCS) $(INCLUDES)
	$(DEPEND) $(CFLAGS) $(INCLUDES) $(SRCS) -f - >Makefile.depend


include Makefile.depend

