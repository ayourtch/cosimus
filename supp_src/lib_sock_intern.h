
/*
 * Copyright (c) Andrew Yourtchenko <ayourtch@gmail.com>, 2008-2009
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Cosimus Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */



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

typedef struct {
  struct pollfd ufds[MAX_FDS];
  conndata_t cdata[MAX_FDS];
  int nfds;
  int nclients;               /* number of active inbound connections */
  //long nconnections;          /* counter for incoming connections */
  int biggest_udp_idx;        /* the biggest index of the UDP sockets */
} lib_sock_runtime_data_t;



#ifdef XXXXX
extern struct pollfd ufds[MAX_FDS];
extern conndata_t cdata[MAX_FDS];
extern int nfds;

int nclients;

long nconnections; 

int biggest_udp_idx; 
#endif
int do_l7_reset(int idx);



#endif

