
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


#ifndef __LIB_SOCK_H__
#define __LIB_SOCK_H__

#include "lib_poll.h"
#include "lib_dbuf.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <errno.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "libsupp.h"


/**** Event handler function pointer types */
typedef void (*sock_ev_channel_ready_t)(int idx, void *u_ptr);
typedef int (*sock_ev_read_t)(int idx, dbuf_t *d, void *u_ptr);
typedef int (*sock_ev_closed_t)(int idx, void *u_ptr);
typedef int (*sock_ev_newconn_t)(int idx, int parent, void *u_ptr);

typedef struct sock_handlers_t_tag {
  sock_ev_channel_ready_t ev_channel_ready;
  sock_ev_read_t ev_read;
  sock_ev_closed_t ev_closed;
  sock_ev_newconn_t ev_newconn;
} sock_handlers_t;

void print_socks(void);

int bind_tcp_listener_specific(char *addr, int port);
int bind_tcp_listener(int port);

int sock_one_cycle(int timeout, void *u_ptr);

int init_ssl(char *cert_file_svr, char *key_file_svr, char *cert_file_clt,
         char *key_file_clt);

dbuf_t *cdata_get_appdata_dbuf(int idx, const char *appdata_sig);
void cdata_set_appdata_dbuf(int idx, dbuf_t *d);

int cdata_get_remote4(int idx, uint32_t *addr, uint16_t *port);
int cdata_check_remote4(int idx, uint32_t addr, uint16_t port);
// immediate send. Use only for the UDP sockets, preferrably. i == idx
int sock_send_data(int i, dbuf_t * d);



sock_handlers_t *cdata_get_handlers(int idx);

void close_idx(int idx, void *u_ptr);

int bind_udp_listener_specific(char *addr, int port, char *remote);

void pkt_dprint_cdata_all(dbuf_t * d);

void *libsock_init(void *data);


#endif
