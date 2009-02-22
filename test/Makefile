include ../Makefile.inc

all: tst

tst: tst.o
	$(CC) $(LDFLAGS) -o tst $^ $(LIBS)

clean:
	rm -f *.o tst
