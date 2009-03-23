include ../Makefile.inc

all: libpktsmv.so

libpktsmv.so: pktsmv.o
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^ $(LIBS) -lfmv
	cp $@ ../lib/$@.tmp
	mv ../lib/$@.tmp ../lib/$@


clean:
	rm -f *.o libpktsmv.so
