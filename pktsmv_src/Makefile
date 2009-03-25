include ../Makefile.inc

all: libpktsmv.a

libpktsmv.a: pktsmv.o
	$(AR) $@ $?
	$(RANLIB) $@
clean:
	$(RM) *.o libpktsmv.a
