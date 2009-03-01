CC=gcc
CFLAGS = -Wall -Werror -I../supp_src -I../fmv_src
LDFLAGS = -rdynamic 
CFLAGS += -g 

LIBS = -lssl -lm -L../supp_src -lsupp -L../fmv_src -lfmv

.c.o:
	$(CC) $(CFLAGS) -c $<
