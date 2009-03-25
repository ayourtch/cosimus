CC=gcc
AR= ar rcu
RANLIB= ranlib
RM= rm -f

CFLAGS = -Wall -Werror -I../supp_src -I../fmv_src -I../lua/lua-5.1.4/src -I../pktsmv_src
LDFLAGS = -rdynamic 
CFLAGS += -g 
# -pg
LUA=../lua/lua-5.1.4/src/lua
LUA_LIBS=-L../lua/lua-5.1.4/src -llua

LIBS = -lssl -lm -L../supp_src -L../fmv_src -L../pktsmv_src -lfmv -lpktsmv -lsupp
# -lfmv

.c.o:
	$(CC) $(CFLAGS) -c $<
