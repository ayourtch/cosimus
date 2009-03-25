include ../Makefile.inc

all: clean cosimus


cosimus: cosimus.o
	$(CC) $(CFLAGS) -o $@ $? $(LIBS) $(LUA_LIBS) $(APP_LIBS)
clean:
	$(RM) *.o cosimus
