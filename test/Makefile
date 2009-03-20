include ../Makefile.inc

all: tst tstlib.so libreload.so

tst: tst.o
	$(CC) $(LDFLAGS) -o tst $^ $(LIBS)

tstlib.so: tstlib.o
	$(CC) $(LDFLAGS) -shared -o tstlib.so $^ $(LIBS)

libreload.so: libreload.o
	$(CC) $(LDFLAGS) -shared -o $@ $^ $(LUA_LIBS)

clean:
	rm -f *.o tst
