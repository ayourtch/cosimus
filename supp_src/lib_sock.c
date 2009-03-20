
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

#include "lib_dbuf.h"
#include "lib_debug.h"
#include "lib_lists.h"
/*
#include "lib_timers.h"
*/

#include "lib_sock_intern.h"
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * @defgroup socket Socket handling
 */

/*@{*/

#ifdef XXXXXX
struct pollfd libsock_data->ufds[MAX_FDS];
conndata_t libsock_data->cdata[MAX_FDS];
int libsock_data->nfds = 0;

int nclients = 0;               /* number of active inbound connections */

long nconnections = 0;          /* counter for incoming connections */

int biggest_udp_idx = 0;        /* the biggest index of the UDP sockets */
#endif

lib_sock_runtime_data_t *libsock_data = NULL;

/* L5 stuff */


void
print_cdata(int i, int debuglevel)
{
  debug(DBG_GLOBAL, debuglevel, "CDATA for %d:", i);
  debug(DBG_GLOBAL, debuglevel, "  xmit_list: %x", libsock_data->cdata[i].xmit_list);
  debug(DBG_GLOBAL, debuglevel, "  written: %d", libsock_data->cdata[i].written);
  debug(DBG_GLOBAL, debuglevel, "  listener: %d", libsock_data->cdata[i].listener);
  debug(DBG_GLOBAL, debuglevel, "  listener_port: %d", libsock_data->cdata[i].listen_port);
  debug(DBG_GLOBAL, debuglevel, "  is_udp: %d", libsock_data->cdata[i].is_udp);
  debug(DBG_GLOBAL, debuglevel, "  do_ssl: %d", libsock_data->cdata[i].do_ssl);
  debug(DBG_GLOBAL, debuglevel, "  is_ssl: %d", libsock_data->cdata[i].is_ssl);
  debug(DBG_GLOBAL, debuglevel, "  ssl: %x", libsock_data->cdata[i].ssl);
  debug(DBG_GLOBAL, debuglevel, "  remote: %s:%d",
        inet_ntoa(libsock_data->cdata[i].remote.sin_addr), libsock_data->cdata[i].remote.sin_port);
  debug(DBG_GLOBAL, debuglevel, "  apptype: %d", libsock_data->cdata[i].apptype);
  debug(DBG_GLOBAL, debuglevel, "  appdata: %x", libsock_data->cdata[i].appdata);
  debug(DBG_GLOBAL, debuglevel, "  l7state: %d", libsock_data->cdata[i].l7state);
  debug(DBG_GLOBAL, debuglevel, "  recv_list: %x", libsock_data->cdata[i].recv_list);
}

/* Dump all we know about sockets */

void print_socks(void)
{
  int i;
  for(i = 0; i < libsock_data->nfds; i++) {
    print_cdata(i, 0);
  }
}

void
pkt_dprint_cdata(int i, dbuf_t * d)
{
  dprintf(d,
          "  %d:%s  do_ssl:%d,ssl:%d,listen:%d,lport:%-5d remote:%16s:%-5d fd:%-5d revents:%x\n",
          i, libsock_data->cdata[i].is_udp ? "udp" : "tcp", libsock_data->cdata[i].do_ssl,
          libsock_data->cdata[i].is_ssl, libsock_data->cdata[i].listener, libsock_data->cdata[i].listen_port,
          inet_ntoa(libsock_data->cdata[i].remote.sin_addr), libsock_data->cdata[i].remote.sin_port,
          libsock_data->ufds[i].fd, libsock_data->ufds[i].revents);
}

void
pkt_dprint_cdata_all(dbuf_t * d)
{
  int i;

  for(i = 0; i < libsock_data->nfds; i++) {
    pkt_dprint_cdata(i, d);
  }
}


void
dsend(int idx, dbuf_t * d)
{
  debug(DBG_GLOBAL, 5, "pushing %d bytes for index %d", d->dsize, idx);
  lpush(&libsock_data->cdata[idx].xmit_list, d);
  libsock_data->ufds[idx].events |= POLLOUT;
  // Will be unlocked by the send mechanism
  dlock(d);
}

dbuf_t *
cdata_get_appdata_dbuf(int idx, const char *appdata_sig)
{
  if(libsock_data->cdata[idx].appdata == NULL) {
    return NULL;
  } else {
    if(dcheckusig(libsock_data->cdata[idx].appdata, appdata_sig)) {
      return libsock_data->cdata[idx].appdata;
    } else {
      return NULL;
    }
  }
}

void
cdata_set_appdata_dbuf(int idx, dbuf_t *d)
{
  libsock_data->cdata[idx].appdata = d; 
}

sock_handlers_t *
cdata_get_handlers(int idx)
{
  return &libsock_data->cdata[idx].handlers;
}

int 
cdata_get_remote4(int idx, uint32_t *addr, uint16_t *port) {
  if (addr) {
    *addr = *((uint32_t *) (&libsock_data->cdata[idx].remote.sin_addr)); 
  }
  if (port) {
    *port = (uint16_t) libsock_data->cdata[idx].remote.sin_port;
  }
  return 1;
}

int 
cdata_check_remote4(int idx, uint32_t addr, uint16_t port) {
  uint32_t a;
  uint16_t p;
  cdata_get_remote4(idx, &a, &p);
  return ((a == addr) && (p == port));
}


/*@}*/

/**
 * @defgroup ssl SSL and other "crypto" functionality
 *
 * @{
 */

SSL_METHOD *ssl_client_meth;
SSL_METHOD *ssl_server_meth;
SSL_CTX *ssl_client_ctx;
SSL_CTX *ssl_server_ctx;


int
load_ssl_ctx_certkey(SSL_CTX * ssl_ctx, char *cert_file, char *key_file)
{
  debug(DBG_SSL, 3, "Loading PEM-encoded cert from file %s...", cert_file);
  if(SSL_CTX_use_certificate_file(ssl_ctx, cert_file, SSL_FILETYPE_PEM)
     <= 0) {
    ERR_print_errors_fp(stderr);
    return -1;
  }
  debug(DBG_SSL, 3, "%s successfully loaded", cert_file);

  debug(DBG_SSL, 3, "Loading PEM-encoded private key from file %s...",
        key_file);
  if(SSL_CTX_use_PrivateKey_file(ssl_ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    return -2;
  }
  debug(DBG_SSL, 3, "%s successfully loaded", key_file);
  if(!SSL_CTX_check_private_key(ssl_ctx)) {
    debug(DBG_SSL, 0,
          "Private key from %s does not match the certificate public key from %s",
          key_file, cert_file);
    return -3;
  }
  return 0;
}


int
init_ssl(char *cert_file_svr, char *key_file_svr, char *cert_file_clt,
         char *key_file_clt)
{
  int rc = 0;

  SSLeay_add_ssl_algorithms();
  //ssl_client_meth = SSLv2_client_method();
  ssl_client_meth = SSLv3_client_method();
  ssl_client_ctx = SSL_CTX_new(ssl_client_meth);

  ssl_server_meth = SSLv23_server_method();
  ssl_server_ctx = SSL_CTX_new(ssl_server_meth);
  SSL_load_error_strings();

  if(!ssl_client_ctx || !ssl_server_ctx) {
    ERR_print_errors_fp(stderr);
    return -1;
  }
  debug(DBG_SSL, 1, "Loading the key/cert for the client SSL context...");
  rc = load_ssl_ctx_certkey(ssl_client_ctx, cert_file_clt, key_file_clt);
  if(rc < 0) {
    return rc - 10;
  }
  debug(DBG_SSL, 1, "SSL client context successfully loaded");
  debug(DBG_SSL, 1, "Loading the key/cert for the server SSL context...");

  rc = load_ssl_ctx_certkey(ssl_server_ctx, cert_file_svr, key_file_svr);
  if(rc < 0) {
    return rc - 20;
  }
  debug(DBG_SSL, 1, "SSL server context successfully loaded");

  return 0;
}

int
test_verify(int ok, X509_STORE_CTX * ctx)
{
  debug(DBG_SSL, 0, "Verify: %d", ok);
  return 1;
}

/** 
 * called on an cleartext socket after accept, 
 * if we want to do the SSL nego on this socket 
 */
int
ssl_negotiate(int idx, int is_server)
{
  int err = 0;
  X509 *client_cert;
  char *str;

  if(libsock_data->cdata[idx].is_ssl) {
    debug(DBG_SSL, 0,
          "ERROR: trying to start SSL for index %d, when already started - assuming success",
          idx);
    print_cdata(idx, 0);
    return 1;
  }
  if(libsock_data->cdata[idx].is_udp) {
    debug(DBG_SSL, 0, "ERROR: trying to start SSL for index UDP %d", idx);
    print_cdata(idx, 0);
    return 0;
  }

  if(libsock_data->cdata[idx].ssl == NULL) {
    libsock_data->cdata[idx].ssl = SSL_new(ssl_server_ctx);
    if(libsock_data->cdata[idx].ssl == NULL) {
      debug(DBG_SSL, 0, "Could not create SSL for index %d", idx);
      return -100;
    }
    err = SSL_set_fd(libsock_data->cdata[idx].ssl, libsock_data->ufds[idx].fd);
    if(err == 0) {
      debug(DBG_SSL, 0, "Could not assign socket to SSL for index %d", idx);
      SSL_free(libsock_data->cdata[idx].ssl);
      libsock_data->cdata[idx].ssl = NULL;
      return -101;
    }
    SSL_set_verify(libsock_data->cdata[idx].ssl,
                   SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, test_verify);
  }
  if(is_server) {
    err = SSL_accept(libsock_data->cdata[idx].ssl);
    debug(DBG_SSL, 2, "Error from SSL_accept: %d", err);
  } else {
    err = SSL_connect(libsock_data->cdata[idx].ssl);
    debug(DBG_SSL, 2, "Error from SSL_connect: %d", err);
  }
  if(err <= 0) {
    ERR_print_errors_fp(stderr);
    return err;
  }
  debug(DBG_SSL, 2, "Index %d: SSL connection with cipher %s",
        idx, SSL_get_cipher(libsock_data->cdata[idx].ssl));
  // Get the client cert and compare it if needed. (NB: dynamic allocation) 
  client_cert = SSL_get_peer_certificate(libsock_data->cdata[idx].ssl);
  if(client_cert != NULL) {
    debug(DBG_SSL, 10, "Peer certificate:");
    str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
    if(str != NULL) {
      debug(DBG_SSL, 10, "  subject: %s", str);
      free(str);
    }
    str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
    if(str != NULL) {
      debug(DBG_SSL, 10, "  issuer: %s", str);
      free(str);
    }
    // some checks on client cert go here 
    X509_free(client_cert);
  } else {
    debug(DBG_SSL, 1, "Peer does not have a certificate");
  }
  // all SSL things done, now consider socket as SSL 
  libsock_data->cdata[idx].is_ssl = 1;
  return 1;
}

/*@}*/


/**
 * @addtogroup socket 
 */

/*@{*/

/*
int
do_l7_reset(int idx)
{
  dbuf_t *d;

  while((d = rpop(&libsock_data->cdata[idx].recv_list))) {
    dunlock(d);
  }
  debug(DBG_GLOBAL, 1, "L7 reset");
  libsock_data->cdata[idx].l7state = L7_INIT;
  return 1;
}
*/

/**
 * Gets called when the channel with the idx is ready - once.
 */
void
ev_channel_ready(int idx, void *u_ptr)
{
  if (libsock_data->cdata[idx].handlers.ev_channel_ready) {
    libsock_data->cdata[idx].handlers.ev_channel_ready(idx, u_ptr);
  }
}

/**
 * Event function which fires when 
 * the data in the dbuf has been read from the socket with index idx 
 */
int
ev_read(int idx, dbuf_t * d, void *u_ptr)
{
  int out = 0;

  debug(DBG_GLOBAL, 1, "ev_read for index %d idx, apptype: %d\n",
        idx, libsock_data->cdata[idx].apptype);
  debug_dump(DBG_GLOBAL, 100, d->buf, d->dsize);

  if (libsock_data->cdata[idx].handlers.ev_read) {
    out = libsock_data->cdata[idx].handlers.ev_read(idx, d, u_ptr);
  }
  return out;
}

/**
 * the socket in index idx got closed 
 */
int
ev_closed(int idx, void *u_ptr)
{
  dbuf_t *d;

  if(libsock_data->cdata[idx].is_ssl) {
    SSL_shutdown(libsock_data->cdata[idx].ssl);
    SSL_free(libsock_data->cdata[idx].ssl);
    libsock_data->cdata[idx].ssl = NULL;
  }
  while((d = rpop(&libsock_data->cdata[idx].xmit_list))) {
    dunlock(d);
  }
  if(libsock_data->cdata[idx].handlers.ev_closed) {
    libsock_data->cdata[idx].handlers.ev_closed(idx, u_ptr);
  }
  dunlock(libsock_data->cdata[idx].appdata);

  if(libsock_data->cdata[idx].inbound) {
    libsock_data->nclients--;
    /*
       if(nclients == 0) {
       debug(DBG_GLOBAL, 1, "Last client went away, close the outbound connection");
       close(libsock_data->ufds[delayed_idx].fd);
       libsock_data->ufds[delayed_idx].fd = -1;
       }
     */
  }
  return 1;
}

/**
 * the connect initiated for socket idx has failed 
 */
int
ev_connectfailed(int idx)
{
  return 1;
}

/**
 * init the data for an index. 
 */
int
init_idx(int idx)
{
  libsock_data->ufds[idx].events = POLLIN;
  memset(&libsock_data->cdata[idx], 0, sizeof(libsock_data->cdata[idx]));
#if 0
  libsock_data->cdata[idx].xmit_list = NULL;
  libsock_data->cdata[idx].written = 0;
  libsock_data->cdata[idx].recv_list = NULL;
  libsock_data->cdata[idx].apptype = 0;
  libsock_data->cdata[idx].http_path = NULL;
  libsock_data->cdata[idx].http_querystring = NULL;
  libsock_data->cdata[idx].http_referer = NULL;
  libsock_data->cdata[idx].listener = 0;
  libsock_data->cdata[idx].http_11 = 0;
#endif
  return 1;

}

void
close_idx(int idx, void *u_ptr)
{
  ev_closed(idx, u_ptr);
  close(libsock_data->ufds[idx].fd);
  libsock_data->ufds[idx].fd = -1;
}


/** 
 * initiate the connect to a given remote addr, return the index 
 */
int
initiate_connect(char *addr, int port)
{
  int s;
  struct sockaddr_in sin;
  int result;
  int idx = -1;
  struct hostent *hp;

  assert(libsock_data->nfds <= MAX_FDS);      /* we should never go _above_ */
  if(libsock_data->nfds == MAX_FDS) {
    printf
      ("Too many sockets opened, can not initiate outbound connection to %s:%d!",
       addr, port);
    return -1;
  }
  s = socket(PF_INET, SOCK_STREAM, 6);
  if(s < 0) {
    printf("Could not allocate socket!");
    return -1;
  }
  bzero(&sin, sizeof(sin));
  sin.sin_port = htons(port);
  sin.sin_family = AF_INET;
  inet_aton(addr, &sin.sin_addr);
  hp = gethostbyname(addr);
  sin.sin_addr.s_addr = ((struct in_addr *) (hp->h_addr))->s_addr;

  fcntl(s, F_SETFL, O_NONBLOCK);
  result = connect(s, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));
  if(result == 0) {
    printf("Successful connect to %s:%d on a nonblocking socket ??",
           addr, port);
    return -1;
  } else {
    if(errno == EINPROGRESS) {
      init_idx(libsock_data->nfds);
      libsock_data->ufds[libsock_data->nfds].fd = s;
      /*
         the socket connection once completed, appears to cause the POLLIN rather than pollout event 
       */
      libsock_data->ufds[libsock_data->nfds].events |= POLLOUT;     /* we want to know when we get connected */
      libsock_data->cdata[libsock_data->nfds].connected = 0;        /* we did not connect yet */
      libsock_data->cdata[libsock_data->nfds].inbound = 0;  /* outbound connection */
      idx = libsock_data->nfds;
      debug(DBG_GLOBAL, 1,
            "Outbound connection in progress for index %d to %s:%d",
            idx, addr, port);
      libsock_data->nfds++;

    } else {
      debug(DBG_GLOBAL, 1,
            "Unexpected error on connection attempt for index %d: %s",
            idx, strerror(errno));
      return -1;
    }

  }
  return 0;
}




/**
 * new connection accepted, the index of it is idx 
 */
int
ev_newconn(int idx, int parent, void *u_ptr)
{
  bzero(&libsock_data->cdata[idx], sizeof(libsock_data->cdata[idx]));

  libsock_data->cdata[idx].connected = 1;
  libsock_data->cdata[idx].inbound = 1;
  libsock_data->cdata[idx].l7state = 0;
  libsock_data->cdata[idx].recv_list = 0;


  debug(DBG_GLOBAL, 1,
        "New connection on index %d, connected: %d, inbound: %d", idx,
        libsock_data->cdata[idx].connected, libsock_data->cdata[idx].inbound);

  if(libsock_data->cdata[parent].handlers.ev_newconn) {
     libsock_data->cdata[parent].handlers.ev_newconn(idx, parent, u_ptr); 
  }
  if(libsock_data->cdata[idx].do_ssl == 0) {
    if(libsock_data->cdata[idx].handlers.ev_channel_ready) {
      libsock_data->cdata[idx].handlers.ev_channel_ready(idx, u_ptr);
    }
  }
  return 0;
}


int
create_tcp_socket()
{
  int s;
  long yes = 1;

  s = socket(PF_INET, SOCK_STREAM, 6);
  notminus(s, "could not create socket");
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  return s;
}

int
create_udp_socket()
{
  int s;
  long yes = 1;

  s = socket(PF_INET, SOCK_DGRAM, 0);
  notminus(s, "could not create socket");
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  return s;
}

/**
 * get a new free index
 */
int
sock_get_free_index(int minimum)
{
  int i;

  for(i = minimum; i < libsock_data->nfds; i++) {
    if(libsock_data->ufds[i].fd == -1) {
      return i;
    }
  }
  i = libsock_data->nfds;
  libsock_data->nfds++;
  return i;
}


/**
 * Bind the socket s to local address and port
 *
 * @param s socket fd
 * @param addr local address to bind to
 * @param port local port to bind to
 *
 * @return index of the newly bound socket within cdata/ufds structures
 *
 * @see bind_udp_listener_specific
 */

int
bind_socket_listener_specific(int newidx, int s, char *addr, int port)
{
  struct sockaddr_in sin;
  int idx = newidx;

  bzero(&sin, sizeof(sin));
  sin.sin_port = htons(port);
  if(addr != NULL) {
    inet_aton(addr, &sin.sin_addr);
  }

  notminus(bind(s, (struct sockaddr *) &sin, sizeof(sin)), "bind failed");

  init_idx(idx);
  libsock_data->cdata[idx].listener = 1;
  libsock_data->cdata[idx].listen_port = port;

  libsock_data->ufds[idx].fd = s;
  libsock_data->ufds[idx].events = POLLIN;
  debug(DBG_GLOBAL, 1,
        "Index %d - starting listening on port %d for address %s!", idx,
        port, inet_ntoa(sin.sin_addr));
  return idx;
}

int
bind_udp_listener_specific(char *addr, int port, char *remote)
{
  struct sockaddr_in sin;
  struct hostent *hp;
  int s = create_udp_socket();
  int idx =
    bind_socket_listener_specific(sock_get_free_index(libsock_data->biggest_udp_idx), s,
                                  addr, port);

  libsock_data->biggest_udp_idx = idx;

  libsock_data->cdata[idx].is_udp = 1;

  bzero(&sin, sizeof(sin));
  sin.sin_port = 0;             //7777; // htons(port);
  sin.sin_family = AF_INET;
  //inet_aton(remote , &sin.sin_addr);

  fcntl(s, F_SETFL, O_NONBLOCK);

  debug(DBG_GLOBAL, 1, "UDP listener on %s:%d started with index %d!",
        addr, port, idx);
  if(remote != NULL) {
    hp = gethostbyname(remote);
    sin.sin_addr.s_addr = ((struct in_addr *) (hp->h_addr))->s_addr;
    notminus(connect
             (s, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)),
             "Connect of UDP");
  }
  // even though it is UDP, we assume it is "connected"
  libsock_data->cdata[idx].connected = 1;
  libsock_data->cdata[idx].inbound = 1;
  return idx;
}



int
bind_tcp_listener_specific(char *addr, int port)
{
  int s = create_tcp_socket();
  int idx =
    bind_socket_listener_specific(sock_get_free_index(0), s, addr, port);

  libsock_data->cdata[idx].is_udp = 0;
  notminus(listen(s, 3), "listen() failed");
  debug(DBG_GLOBAL, 1, "TCP listener on %s:%d started with index %d!",
        addr, port, idx);
  return idx;
}

int
bind_tcp_listener(int port)
{
  return bind_tcp_listener_specific(NULL, port);
}

/*@}*/


/**
 * POLLIN event on the TCP listening socket - means there is a new
 * incoming connection, that we need to accept, allocate a new index 
 * for it, and call the appropriate handlers.
 *
 * @param i index of the listening socket
 *
 * @see sock_unconnected_pollin
 */

void
sock_tcp_listener_pollin(int i, void *u_ptr)
{
  struct sockaddr_in sin1;
  unsigned int sin1sz = sizeof(sin1);
  //int revents_save = libsock_data->ufds[i].revents;

  libsock_data->ufds[i].revents = 0;
  debug(DBG_GLOBAL, 1, "before accept");
  int s1 = accept(libsock_data->ufds[i].fd, (struct sockaddr *) &sin1,
                  &sin1sz);
  int newidx = sock_get_free_index(0);

  debug(DBG_GLOBAL, 1, "after accept");
  debug(DBG_GLOBAL, 1,
        "New inbound connection on index %d from %s:%d -> trying to use index %d",
        i, inet_ntoa(sin1.sin_addr), ntohs(sin1.sin_port), newidx);
  debug(DBG_GLOBAL, 10, "%d:LISTEN-POLLIN", i);
  if(libsock_data->nfds < MAX_FDS) {
    fcntl(s1, F_SETFL, O_NONBLOCK);
    init_idx(newidx);
    libsock_data->ufds[newidx].fd = s1;
    if(ev_newconn(newidx, i, u_ptr) < 0) {
      debug(DBG_GLOBAL, 1,
            "Application-specific error, closing the connection");
      close(s1);
      libsock_data->ufds[newidx].fd = -1;
      libsock_data->ufds[newidx].revents = 0;
    }
  } else {
    debug(DBG_GLOBAL, 1,
          "Could not accept new connection, too many open sockets!");
    close(s1);
  }
  //libsock_data->ufds[i].revents = revents_save;
}

/**
 * POLLIN event on the unconnected socket - this is supposed to 
 * be the connection result for the async outbound connection.
 *
 * @param i index that experienced POLLIN event
 *
 * @see sock_tcp_listener_pollin
 */
void
sock_unconnected_pollinout(int i, void *u_ptr)
{
  int err;
  socklen_t l = sizeof(err);

  if(getsockopt(libsock_data->ufds[i].fd, SOL_SOCKET, SO_ERROR, &err, &l) == -1) {
    debug(DBG_GLOBAL, 1, "Getsockopt failure (%s)!", strerror(errno));
  }
  if(err != 0) {
    debug(DBG_GLOBAL, 1,
          "Outbound connection error for index %d: %s", i, strerror(err));
    ev_connectfailed(i);
    close(libsock_data->ufds[i].fd);
    libsock_data->ufds[i].fd = -1;
  } else {
    debug(DBG_GLOBAL, 1, "Index %d successfully connected", i);
    libsock_data->cdata[i].connected = 1;
    ev_newconn(i, -1, u_ptr);
  }
}

void
sock_ssl_check_error(int i, int ret, void *u_ptr)
{
  int err;
  int err2;

  switch (err = SSL_get_error(libsock_data->cdata[i].ssl, ret)) {
  case SSL_ERROR_NONE:
    libsock_data->ufds[i].events = POLLIN;
    break;
  case SSL_ERROR_ZERO_RETURN:
    close_idx(i, u_ptr);
    break;
  case SSL_ERROR_WANT_READ:
    libsock_data->ufds[i].events |= POLLIN;
    break;
  case SSL_ERROR_WANT_WRITE:
    libsock_data->ufds[i].events |= POLLOUT;
    break;
  case SSL_ERROR_SYSCALL:
    err2 = ERR_get_error();
    debug(DBG_SSL, 0, "SSL error: syscall error %d on index %d", err2, i);
    close_idx(i, u_ptr);
    break;
  default:
    debug(DBG_SSL, 0, "SSL error: %d, closing index %d", err, i);
    ERR_print_errors_fp(stderr);
    close_idx(i, u_ptr);
  }
}

/**
 * POLLIN event for the SSL socket that is in the process of negotiation
 *
 * @param i index of the socket
 */
void
sock_ssl_pollinout(int i, void *u_ptr)
{
  int ret;

  if((ret = ssl_negotiate(i, libsock_data->cdata[i].inbound)) == 1) {
    libsock_data->cdata[i].do_ssl = 0;
    debug(DBG_GLOBAL, 11, "SSL done!! (idx %d)", i);
    libsock_data->ufds[i].events = POLLIN;
    ev_channel_ready(i, u_ptr);
  } else {
    sock_ssl_check_error(i, ret, u_ptr);
  }

}

int
sock_receive_data(int i, dbuf_t * d)
{
  if(libsock_data->cdata[i].is_udp == 1) {
    socklen_t remote_len = sizeof(libsock_data->cdata[i].remote);

    d->dsize =
      recvfrom(libsock_data->ufds[i].fd, d->buf, d->size, 0,
               (struct sockaddr *) &libsock_data->cdata[i].remote, &remote_len);
  } else {
    if(libsock_data->cdata[i].is_ssl) {
      d->dsize = SSL_read(libsock_data->cdata[i].ssl, d->buf, d->size);
    } else {
      d->dsize = read(libsock_data->ufds[i].fd, d->buf, d->size);
    }
  }
  libsock_data->cdata[i].rx_count++;
  return d->dsize;
}

/**
 * POLLIN event on connected socket *or* on a UDP socket
 *
 * @arg i index of the socket
 *
 * @see sock_connected_pollout
 * @see sock_unconnected_pollinout
 * @see sock_tcp_listener_pollin
 */

void
sock_connected_pollin(int i, void *u_ptr)
{
  if(libsock_data->cdata[i].do_ssl) {
    sock_ssl_pollinout(i, u_ptr);
  } else {
    dbuf_t *d = NULL;

    debug(DBG_GLOBAL, 11, "..on a Xconnected socket (idx %d)", i);
    d = dalloc(CHUNK_SZ);
    debug(DBG_GLOBAL, 11, "sock_connected_pollin: new dbuf %x (idx %d)", d,
          i);
    if(d != NULL) {
      sock_receive_data(i, d);
      debug(DBG_GLOBAL, 11,
            "..read %d bytes - max %d (idx %d)", d->dsize, d->size, i);
      if(d->dsize == 0 || (d->dsize > 0 && ev_read(i, d, u_ptr) == 0)) {
        debug(DBG_GLOBAL, 1, "Index %d closed by remote host", i);
        close_idx(i, u_ptr);
      }
      // those who needed this have copied it already 
      debug(DBG_GLOBAL, 11, "sock_connected_pollin: freeing buffer %x", d);
      dunlock(d);
    } else {
      debug(DBG_GLOBAL, 11, ".. NOT allocated chunk (idx %d)", i);
    }
  }
}

/**
 * Attempt to send some data and return the number of bytes written.
 *
 */
int
sock_send_data(int i, dbuf_t * d)
{
  int nwrote;

  if(libsock_data->cdata[i].is_udp) {
    nwrote =
      sendto(libsock_data->ufds[i].fd,
             &d->buf[libsock_data->cdata[i].written],
             d->dsize - libsock_data->cdata[i].written,
             0, (struct sockaddr *) &libsock_data->cdata[i].remote,
             sizeof(libsock_data->cdata[i].remote));
  } else {
    if(libsock_data->cdata[i].is_ssl) {
      nwrote =
        SSL_write(libsock_data->cdata[i].ssl,
                  &d->buf[libsock_data->cdata[i].written], d->dsize - libsock_data->cdata[i].written);
    } else {
      nwrote =
        write(libsock_data->ufds[i].fd,
              &d->buf[libsock_data->cdata[i].written], d->dsize - libsock_data->cdata[i].written);
    }
  }
  return nwrote;
}

/**
 * POLLOUT on the connected socket or on UDP socket
 *
 * @arg i index of the socket
 *
 * @see sock_connected_pollin
 */
void
sock_connected_pollout(int i, void *u_ptr)
{
  if(libsock_data->cdata[i].do_ssl) {
    sock_ssl_pollinout(i, u_ptr);
  } else {
    if(libsock_data->cdata[i].xmit_list != NULL) {
      dbuf_t *d = rpeek(&libsock_data->cdata[i].xmit_list);
      int nwrote;

      if(d->dsize > 0) {
        if(libsock_data->cdata[i].is_udp && libsock_data->cdata[i].rx_count == 0) {
          nwrote = d->dsize - libsock_data->cdata[i].written;
          debug(DBG_GLOBAL, 1, "UDP index %d not connected yet, discard data",
                i);
        } else {
          nwrote = sock_send_data(i, d);
        }
        debug(DBG_GLOBAL, 1, "written: %d out of %d", nwrote, d->dsize);
        if(nwrote + libsock_data->cdata[i].written == d->dsize) {
          debug(DBG_GLOBAL, 1, "done writing");
          dunlock(d);
          rpop(&libsock_data->cdata[i].xmit_list);    /* and delete the item off the list */
          libsock_data->cdata[i].written = 0; /* did not write anything yet from the next one */
          libsock_data->cdata[i].tx_count++;
        } else if(nwrote < d->dsize) {
          // Retry to write more data later
          libsock_data->cdata[i].written += nwrote;
          debug(DBG_GLOBAL, 1, "retry writing later");
        }
      } else {
        // empty data buffer means time to close.
        debug(DBG_GLOBAL, 10, "Seen empty xmit buf - closing index %d", i);
        close_idx(i, u_ptr);
      }
      if(libsock_data->cdata[i].xmit_list == NULL && 0) {
        debug(DBG_GLOBAL, 10, "Nothing left to send - closing index %d", i);
        close_idx(i, u_ptr);
      }
    }
    /*
     * if the list was or has become empty - we're not 
     * interested in knowing when we can write anymore
     */
    if(libsock_data->cdata[i].xmit_list == NULL && libsock_data->cdata[i].do_ssl == 0) {
      libsock_data->ufds[i].events &= ~POLLOUT;
      // TODO: also handling of the keepalive timer ? 
      /*
         debug(DBG_GLOBAL, 1, "Index %d was without keepalive, closing.", i);
         close(libsock_data->ufds[i].fd); libsock_data->ufds[i].fd = -1; 
       */
    }
  }
}

/**
 * Try to compact the libsock_data->nfds and cdata structures,
 * under the assumption that something got closed.
 * TODO: make this conditional.
 */

void
sock_remove_closed_fds(void)
{
  int i;

  for(i = libsock_data->nfds - 1; i >= 0; i--) {
    if(libsock_data->ufds[i].fd == -1) {
      libsock_data->ufds[i].events = 0;
      if(libsock_data->ufds[i].fd == libsock_data->nfds - 1) {
        libsock_data->nfds--;
      }
    }
  }
#if 0
  for(i = 0; i < libsock_data->nfds; i++) {
    if(libsock_data->ufds[i].fd == -1) {
      while(libsock_data->ufds[libsock_data->nfds - 1].fd == -1 && libsock_data->nfds > 0 && i < libsock_data->nfds) {
        libsock_data->nfds--;
      }
      if(i < libsock_data->nfds) {
        libsock_data->ufds[i].fd = libsock_data->ufds[libsock_data->nfds - 1].fd;
        libsock_data->ufds[i].events = libsock_data->ufds[libsock_data->nfds - 1].events;
        memcpy(&libsock_data->cdata[i], &libsock_data->cdata[libsock_data->nfds - 1], sizeof(libsock_data->cdata[0]));  /* also move the connection data */
        libsock_data->nfds--;
      }
    }
  }
#endif
}

int sock_one_cycle(int timeout, void *u_ptr) {
    int i;
    int nevents = 0;

    debug(DBG_GLOBAL, 125, "poll(%d)", libsock_data->nfds);
    poll(libsock_data->ufds, libsock_data->nfds, timeout);
    for(i = 0; i < libsock_data->nfds; i++) {
      if(libsock_data->cdata[i].listener && libsock_data->cdata[i].is_udp == 0) {
        // TCP listener sockets 
        if(libsock_data->ufds[i].revents & POLLIN) {
          sock_tcp_listener_pollin(i, u_ptr);
	  nevents++;
        }
      } else {
        // non-listener sockets - this includes UDP
        if(libsock_data->ufds[i].revents & POLLIN) {
          debug(DBG_GLOBAL, 10, "%d: POLLIN", i);
          if(libsock_data->cdata[i].connected == 0) {
            sock_unconnected_pollinout(i, u_ptr);
          } else {
            sock_connected_pollin(i, u_ptr);
          }
	  nevents++;
        }
        if(libsock_data->ufds[i].revents & POLLOUT) {
          debug(DBG_GLOBAL, 10, "%d: POLLOUT", i);
          if(libsock_data->cdata[i].connected == 0) {
            debug(DBG_GLOBAL, 11, "..on a nonconnected socket...");
            sock_unconnected_pollinout(i, u_ptr);
          } else {
            debug(DBG_GLOBAL, 11, "on connected...");
            sock_connected_pollout(i, u_ptr);
          }
	  nevents++;
        }
        if(libsock_data->ufds[i].revents & POLLERR) {
          debug(DBG_GLOBAL, 11, "%d: POLLERR", i);
        }
        if(libsock_data->ufds[i].revents & POLLHUP) {
          debug(DBG_GLOBAL, 11, "%d: POLLHUP", i);
        }
        if(libsock_data->ufds[i].revents & POLLNVAL) {
          debug(DBG_GLOBAL, 11, "%d: POLLNVAL", i);
        }
        libsock_data->ufds[i].revents = 0;
      }
    }
    sock_remove_closed_fds();
    return nevents;
}

void *libsock_init(void *data)
{
  if(data == NULL) {
    data = malloc(sizeof(libsock_data));
  }
  if(data != NULL) {
    libsock_data = data;
  }
  return data;
}

