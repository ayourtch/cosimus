include ../Makefile.inc

all: cosimus

cosimus: cosimus.o
	$(CC) -o $@ $? $(LIBS) $(LUA_LIBS) $(APP_LIBS)
clean:
	$(RM) *.o cosimus
