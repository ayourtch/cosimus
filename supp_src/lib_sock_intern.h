

/**************************************************************************
*
*  Copyright © 2008-2009 Andrew Yourtchenko, ayourtch@gmail.com.
*
*  Permission is hereby granted, free of charge, to any person obtaining 
* a copy of this software and associated documentation files (the "Software"), 
* to deal in the Software without restriction, including without limitation 
* the rights to use, copy, modify, merge, publish, distribute, sublicense, 
* and/or sell copies of the Software, and to permit persons to whom 
* the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included 
* in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
* OR OTHER DEALINGS IN THE SOFTWARE. 
*
*****************************************************************************/
#ifndef __LIB_SOCK_INTERN__
#define __LIB_SOCK_INTERN__

#include "lib_sock.h"

/* socket store for the poll() */
enum {
  MAX_FDS = 10000,
  CHUNK_SZ = 20480,
};


typedef struct {
  listitem_t *xmit_list;        /* list of to-be transmitted dbuf's for this socket */
  int written;                  /* number of bytes written from the rightmost dbuf, if it was not fully written */
  int connected;
  int inbound;                  /* we accepted this connection rather than initiated it */
  int listener;                 /* this is a listener socket */
  int listen_port;              /* which port are we listening on */

  int is_udp;                   /* =1 if this is a udp socket, else 0 */
  int rx_count;
  int tx_count;

  int do_ssl;                   /* =1 if we need to negotiate the SSL still (only valid for TCP) */
  int is_ssl;                   /* =1 if this is ssl-protected socket with a finished handshake, else 0. only valid for TCP */
  SSL *ssl;                     /* the SSL thing to write to/read from, should be valid if is_ssl is set */
  struct sockaddr_in remote;    /* used for udp sockets - to store rem ip / port */
  int apptype;                  /* application type */
  dbuf_t *appdata;              /* per-session application-specific data */
  int l7state;
  listitem_t *recv_list;        /* list of received to-be-parsed data for this socket */
  sock_handlers_t handlers;     /* structure with the function pointers for the handlers */
} conndata_t;



extern struct pollfd ufds[MAX_FDS];
extern conndata_t cdata[MAX_FDS];
extern int nfds;

int nclients;

long nconnections; 

int biggest_udp_idx; 

int do_l7_reset(int idx);



#endif

