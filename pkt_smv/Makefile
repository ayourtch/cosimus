include ../Makefile.inc

all: pkt-smv.so

pkt-smv.so: pkt-smv.o
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^ $(LIBS) -lfmv
	cp $@ ../lib/$@.tmp
	mv ../lib/$@.tmp ../lib/$@


clean:
	rm -f *.o pkt-smv.so
