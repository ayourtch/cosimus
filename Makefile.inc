CC=gcc
CFLAGS = -Wall -Werror -I../supp_src -I../fmv_src
LDFLAGS = -rdynamic 
CFLAGS += -g 

LIBS = -lssl -lm -L../supp_src -lsupp

.c.o:
	$(CC) $(CFLAGS) -c $<
