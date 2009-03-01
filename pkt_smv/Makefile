include ../Makefile.inc

all: pkt-smv

pkt-smv: pkt-smv.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o pkt-smv
