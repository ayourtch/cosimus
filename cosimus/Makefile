include ../Makefile.inc

all: clean cosimus


cosimus: cosimus.o
	$(CC) -o $@ $? $(LIBS) $(LUA_LIBS) $(APP_LIBS)
clean:
	$(RM) *.o cosimus
