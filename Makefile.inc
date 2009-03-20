CC=gcc
CFLAGS = -Wall -Werror -I../supp_src -I../fmv_src -I../lua/lua-5.1.4/src
LDFLAGS = -rdynamic 
CFLAGS += -g 
LUA=../lua/lua-5.1.4/src/lua
LUA_LIBS=-L../lua/lua-5.1.4/src -llua

LIBS = -lssl -lm -L../supp_src -lsupp -L../fmv_src 
# -lfmv

.c.o:
	$(CC) $(CFLAGS) -c $<
