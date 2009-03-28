

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

enum {
  SOCK_EVENT_CHANNEL_READY = 1,
  SOCK_EVENT_READ = 2,
  SOCK_EVENT_CLOSED = 3,
  SOCK_EVENT_NEWCONN = 4
};


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
int initiate_connect(char *addr, int port);


int sock_one_cycle(int timeout, void *u_ptr);

int init_ssl(char *cert_file_svr, char *key_file_svr, char *cert_file_clt,
         char *key_file_clt);

dbuf_t *cdata_get_appdata_dbuf(int idx, const char *appdata_sig);
void cdata_set_appdata_dbuf(int idx, dbuf_t *d);

int cdata_get_remote4(int idx, uint32_t *addr, uint16_t *port);
int cdata_check_remote4(int idx, uint32_t addr, uint16_t port);
// immediate send. Use only for the UDP sockets, preferrably. i == idx
int sock_send_data(int i, dbuf_t * d);

// immediate send, if can not send then enqueues. Returns the number of bytes immediately sent
int sock_write_data(int i, dbuf_t * d);



sock_handlers_t *cdata_get_handlers(int idx);

void close_idx(int idx, void *u_ptr);

int bind_udp_listener_specific(char *addr, int port, char *remote);

void pkt_dprint_cdata_all(dbuf_t * d);


#endif
