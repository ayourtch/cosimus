#ifndef _APP_HTTP_H_
#define _APP_HTTP_H_

#include "lib_dbuf.h"
#include "lib_sock.h"

enum {
  HTTP_REQ_INCOMPLETE = 0,
  HTTP_REQ_UNKNOWN,
  HTTP_REQ_GET,
  HTTP_REQ_POST,
  HTTP_REQ_CONNECT,
  HTTP_L7_INIT,
  HTTP_L7_SHOWTIME,
  HTTP_L7_READ_POST_DATA,
};

struct appdata_http_t_tag;

typedef int (*http_handler_func_t)(dbuf_t *dad, dbuf_t *dh, dbuf_t *dd);
typedef http_handler_func_t (*http_dispatcher_func_t)(dbuf_t *dad);

typedef struct appdata_http_t_tag {
  int l7state;			/* l7 state */
  dbuf_t *rdb;			/* request dbuf - the data that is sent from the browser */
  char *http_path;              /* requested path */
  char *http_querystring;       /* query string */
  char *http_referer;
  int http_11;                  /* is it http_1.1 ? */
  uint32_t post_content_length;
  dbuf_t *post_content_buf;
  uint32_t post_content_got_length;
  http_dispatcher_func_t dispatcher;
} appdata_http_t;

dbuf_t *alloc_appdata_http(int idx);
appdata_http_t *http_dbuf_get_appdata(dbuf_t *d);



int ev_http_read(int idx, dbuf_t * d, void *u_ptr);
int ev_http_connect_read(int idx, dbuf_t * d, void *u_ptr);

void httpd_register_handlers(void);

/* the one that is exported */
int http_start_listener(char *addr, int port, http_dispatcher_func_t dispatcher);


#endif
