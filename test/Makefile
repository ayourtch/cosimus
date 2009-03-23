include ../Makefile.inc

all: tst tstlib.so 

tst: tst.o
	$(CC) $(LDFLAGS) -o tst $^ $(LIBS) $(LUA_LIBS)

tstlib.so: tstlib.o
	$(CC) $(LDFLAGS) -shared -o tstlib.so $^ $(LIBS) $(LUA_LIBS)

clean:
	rm -f *.o tst
