include ../Makefile.inc
LIBS=-lssl
SRCS= lib_dbuf.c lib_debug.c lib_hash.c lib_hash_func.c lib_lists.c lib_os.c lib_poll.c lib_sock.c lib_httpd.c lib_timers.c lib_uuid.c uuid_sysdep.c lua_libsu.c
INCLUDES= lib_dbuf.h lib_debug.h lib_hash.h lib_hash_func.h lib_lists.h lib_os.h lib_poll.h lib_sock.h lib_sock_intern.h lib_httpd.h lib_timers.h lib_uuid.h uuid_sysdep.h
OBJS= $(subst .c,.o,$(SRCS))

DEPEND=makedepend

all: supplib

supplib: depend libsupp.so

libsupp.so: $(OBJS)
	 $(CC) -shared -Wl,-soname,libsupp.so -o $@ $^ $(LIBS)
	 cp $@ ../lib/

clean:
	rm -f *.o
	rm -f libsupp.so
	rm -f Makefile.depend

depend: Makefile.depend
	
Makefile.depend: $(SRCS) $(INCLUDES)
	$(DEPEND) $(CFLAGS) $(INCLUDES) $(SRCS) -f - >Makefile.depend


include Makefile.depend

