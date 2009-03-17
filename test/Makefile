include ../Makefile.inc

all: tst tstlib.so

tst: tst.o
	$(CC) $(LDFLAGS) -o tst $^ $(LIBS)

tstlib.so: tstlib.o
	$(CC) $(LDFLAGS) -shared -o tstlib.so $^ $(LIBS)

clean:
	rm -f *.o tst
