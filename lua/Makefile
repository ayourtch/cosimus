all: lua libappslua.a

include ../Makefile.inc

lua:
	tar xzvf lua-5.1.4.tar.gz
	(cd lua-5.1.4; make linux)

luazip-1.2.3/src/luazip.c:
	tar xzvf luazip-1.2.3.tar.gz

luazip.o: luazip-1.2.3/src/luazip.c
	${CC} ${CFLAGS} -c -o $@ $?

libappslua.a: luazip.o
	${AR} $@ $?
	${RANLIB} $@

clean:
	rm -rf lua-5.1.4
	rm -rf luazip-1.2.3
	rm -f *.a
	rm -f *.o

