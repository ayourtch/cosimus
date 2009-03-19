include ../Makefile.inc
LIBS += -L../lua/lua-5.1.4/src -llua -L../lib -lfmv

all: pkt-smv

pkt-smv: pkt-smv.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o pkt-smv
