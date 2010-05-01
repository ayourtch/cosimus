
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "lib_sock.h"
#include "lib_hash.h"
#include "lib_debug.h"
#include "lib_httpd.h"
#include "lib_lists.h"

/**
 * @defgroup http HTTP server routines
 */

/*@{*/

/******* higher level app stuff *********/


/**********************************************************************************************
 **********************************************************************************************

 Webserver HTTP stuff

 **********************************************************************************************
**********************************************************************************************/

/*
char *http_header(hashmap_t hashofheaders, char *hname) {
    int length;
    char *data;

    length = hashmap_entry_by_key(hashofheaders, hname, strlen(hname)+1, (void **)&data);
    if (length > 0) {
        return data;
    } else {
        return NULL;
    }
}
*/

char *http_appdata_sig = "http_appdata_sig";

http_handler_func_t default_http_dispatcher(appdata_http_t *ad)
{
  return NULL;
}

/* new connection came in */
int http_newconn(int idx, int parent, void *u_ptr)
{
  appdata_http_t *ad;
  appdata_http_t *adp;
  sock_handlers_t *h;
  dbuf_t *d, *dp;


  d = alloc_appdata_http(idx);
  cdata_set_appdata_dbuf(idx, d);
  dp = cdata_get_appdata_dbuf(parent, http_appdata_sig);

  h = cdata_get_handlers(idx);
  h->ev_read = ev_http_read;

  adp = (void *)dp->buf;
  ad = (void *)d->buf;

  ad->dispatcher = adp->dispatcher;
  ad->l7state = HTTP_L7_INIT;
  ad->L = adp->L;
  ad->lua_handler_name = adp->lua_handler_name;
  debug(DBG_GLOBAL, 1, "Index %d apptype set to HTTP", idx);
  return 1;
}


/* start a http listener on a given addr(can be nul)/port */
int http_start_listener(char *addr, int port, http_dispatcher_func_t dispatcher)
{
  appdata_http_t *ad;
  dbuf_t *d;
  sock_handlers_t *h;

  int idx = bind_tcp_listener_specific(addr, port);

  assert(idx >= 0);

  h = cdata_get_handlers(idx);
  h->ev_newconn = http_newconn;

  d = alloc_appdata_http(idx);
  ad = (void *)d->buf;
  cdata_set_appdata_dbuf(idx, d);

  if(dispatcher) {
    ad->dispatcher = dispatcher;
  } else {
    ad->dispatcher = NULL;
  }
  return idx;
}

void
dhtml_redirect(dbuf_t * dd, char *url)
{
  dxprintf(dd,
          "<html><head><META HTTP-EQUIV=\"refresh\" content=\"0;URL=%s\"></head>",
          url);
  dxprintf(dd, "<body>");
  dxprintf(dd, "Redirecting to <a href=\"%s\">%s</a>...\n", url, url);
  dxprintf(dd, "</body></html>");
}

dbuf_t *swf_intro_dbuf = NULL;


/* autoincrementing image id */
long image_id = 0;

/* just an autoincrement */
long unique_id = 0;

/*@}*/

void
appdata_http_destructor(void *dbuf)
{
  dbuf_t *d = dbuf;
  appdata_http_t *ad = (void *) d->buf;

  if(ad->http_path) {
    free(ad->http_path);
    ad->http_path = NULL;
  }
  if(ad->http_querystring) {
    free(ad->http_querystring);
    ad->http_querystring = NULL;
  }
  if(ad->http_referer) {
    free(ad->http_referer);
    ad->http_referer = NULL;
  }
  if(ad->content_type) {
    free(ad->content_type);
    ad->content_type = NULL;
  }

  if(ad->post_content_buf) {
    dunlock(ad->post_content_buf);
  }
}


dbuf_t *
alloc_appdata_http(int idx)
{
  dbuf_t *d = dsetusig(dalloczf(sizeof(appdata_http_t)), http_appdata_sig);

  ddestructor(d, appdata_http_destructor);
  return d;
}

appdata_http_t *http_dbuf_get_appdata(dbuf_t *d)
{
  if(d && dcheckusig(d, http_appdata_sig)) {
    return (void *)d->buf;
  } else {
    return NULL;
  }
}

static appdata_http_t *get_appdata(int idx)
{
  return http_dbuf_get_appdata(cdata_get_appdata_dbuf(idx, http_appdata_sig));
}

void
do_l7_reset(int idx)
{
  appdata_http_t *ad = get_appdata(idx);
  debug(DBG_GLOBAL, 1, "L7 reset");
  ad->l7state = HTTP_L7_INIT;
}

#ifdef DEAD_CODE  
  else if(strcmp(ad->http_path, "/") == 0) {
    dxprintf(dd, "<h1>Test page</h1>");
    dxprintf(dd, "<form method='post' action='postaction' enctype='multipart/form-data'><input type='text' name=editor><input type=file name=txt><input type='submit'></form>");
    
  } else if(strcmp(ad->http_path, "/postaction") == 0) {
    dxprintf(dd, "<h1>Post results</h1>");
    dxprintf(dd, "<pre>Content len: %d</pre>", ad->post_content_got_length);
    dxprintf(dd, "<pre>");
    if (ad->post_content_buf) {
      dstrcat(dd, (void *)ad->post_content_buf->buf, ad->post_content_buf->dsize);
    }
    dxprintf(dd, "</pre>");
  } else if(strcmp(ad->http_path, "/status") == 0) {
    dxprintf(dd, "<h1>Some header</h1>");
    dxprintf(dd, "You came from: <b>%s</b>", ad->http_referer);
    dxprintf(dd, "URI you requested: %s\n", ad->http_path);
    dxprintf(dd, "<pre>");
    dxprintf(dd, "QUERY_STRING: %s\n", getenv("QUERY_STRING"));
    //dprint_tree(dd);
    dxprintf(dd, "</pre>");
  } else if(strcmp(ad->http_path, "/version.txt") == 0) {
    /*
       debug(DBG_GLOBAL, 1, "Requested version, version is: %ld", jpeg_screenshot_generation);
       //dxprintf(dh, "Content-Type: text/plain\r\n");
       dxprintf(dd, "%ld\r\n\r\n", jpeg_screenshot_generation);
     */

  } else if(strcmp(ad->http_path, "/screen.jpg") == 0) {
    /*
       debug(DBG_GLOBAL, 2, "Sending jpeg on HTTP to %d", idx);
       if (jpeg_screenshot_dbuf) {
       debug(DBG_GLOBAL, 2, "size: %d", jpeg_screenshot_dbuf->dsize);
       dconcat(dd, jpeg_screenshot_dbuf);
       } else {
       debug(DBG_GLOBAL, 2, "nothing to send yet");
       }
       dxprintf(dh, "Content-Type: image/jpeg\r\n");
     */
  } else if(strstr(ad->http_path, "/go/") == ad->http_path) {
  } else {
    debug(DBG_GLOBAL, 2, "Action not found for path: '%s'", ad->http_path);
  }

/* FIXME
    dxprintf(dh, "Content-Type: %s\r\n",  hdf_get_value(cdata[idx].cgi->hdf, "Response.ContentType", NULL));
    dxprintf(dh, "Content-Length: %d\r\n", dd->dsize);
*/
  //dxprintf(dh, "Transfer-Encoding: chunked\r\n");

#endif // DEAD_CODE

/* 
 * The function that receives the control once the request is recognized 
 * Its responsibility is to prepare the response, queue the data, and return.
 */
int
http_handle_request(int idx)
{
  time_t global_time;
  char timebuf[30];
  int retcode;

  dbuf_t *dh;
  dbuf_t *deof;
  dbuf_t *dad = cdata_get_appdata_dbuf(idx, http_appdata_sig);
  appdata_http_t *ad;

  if (dad) {
    ad = http_dbuf_get_appdata(dad);
  }

  if(ad == NULL) {
    debug(DBG_GLOBAL, 0, "Null appdata for HTTP, idx: %d", idx);
  }

  dh = dalloc(1024);
  dbuf_t *dd = dalloc(1024);

  unique_id++;
  deof = dalloc(10);


  if(ad->dispatcher) {
    http_handler_func_t handler = ad->dispatcher(dad);
    if(handler == NULL) {
      debug(DBG_GLOBAL, 2, "Dispatcher found no action for path: '%s'", ad->http_path);
    } else {
      char *pc = get_symbol_name(handler);
      debug(DBG_GLOBAL, 2, "[Idx %d] Dispatcher found action %s for path: '%s'", idx, pc, ad->http_path);
      free(pc);
      retcode = handler(dad, dh, dd);
      if((dh->dsize < 6) || memcmp(dh->buf, "HTTP/1", 6)) {
        dbuf_t *dh0 = dalloc(dh->size);
        dstrcat(dh0, "HTTP/1.1 200 OK\r\n", -1);
        dstrcat(dh0, "Connection: Close\r\n", -1);
        global_time = time(NULL);
        strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT",
           gmtime(&global_time));
        dxprintf(dh0, "Date: %s\r\n", timebuf);
	dmemcat(dh0, dh->buf, dh->dsize);
	dh->dsize = 0;
	dmemcat(dh, dh0->buf, dh0->dsize);
	dunlock(dh0);
      }
      debug(DBG_GLOBAL, 2, "Retcode[idx %d]: %d", idx, retcode);
    }
  } else {
    dxprintf(dh, "HTTP/1.0 400 Not Found\r\n");
  }
  debug(DBG_GLOBAL, 2, "index %d , Content-length: %d\n", idx, dd->dsize);
  dstrcat(dh, "\r\n", -1);
  dconcat(dh, dd);
  dunlock(dd);
  debug_dump(DBG_GLOBAL, 100, dh->buf, dh->dsize);

  // Now hand it out to transmitter 
  dsend(idx, dh);
  dsend(idx, deof);
  dunlock(dh);
  dunlock(deof);


  return 1;
}

int
http_handle_request_connect(int idx)
{
  //int j;
  time_t global_time;
  char timebuf[30];
  dbuf_t *dh;
  //dbuf_t *deof;
  appdata_http_t *ad = get_appdata(idx);

  if(ad == NULL) {
    debug(DBG_GLOBAL, 0, "Null appdata for HTTP, idx: %d", idx);
  }


  dh = dstrcpy("HTTP/1.1 200 OK\r\n");
  dbuf_t *dd = dalloc(1024);

  dstrcat(dh, "Connection: Keepalive\r\n", -1);
  global_time = time(NULL);
  strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT",
           gmtime(&global_time));
  dxprintf(dh, "Date: %s\r\n", timebuf);
  unique_id++;



  if(strcmp(ad->http_path, "/") == 0) {
  } else {
    debug(DBG_GLOBAL, 2, "Action not found for path: '%s'", ad->http_path);
  }

/* FIXME
    dxprintf(dh, "Content-Type: %s\r\n",  hdf_get_value(cdata[idx].cgi->hdf, "Response.ContentType", NULL));
    dxprintf(dh, "Content-Length: %d\r\n", dd->dsize);
*/
  //dxprintf(dh, "Transfer-Encoding: chunked\r\n");
  debug(DBG_GLOBAL, 2, "index %d , Content-length: %d\n", idx, dd->dsize);
  dstrcat(dh, "\r\n", -1);
  dconcat(dh, dd);
  dunlock(dd);

  // Now hand it out to transmitter 
  dsend(idx, dh);
  dunlock(dh);
  //cdata[idx].apptype = APP_HTTP_CONNECT;


  return 1;
}

// Parse the cookies from the string and stick them into the cgi Cookie.* structure.
//

void
parse_cookies(int idx, char *val)
{
  char *cname = val;
  char *cval;
  char *trailer;
  char name_buf[64];

  while(*cname) {
    while(*cname && isspace(*cname))
      cname++;
    cval = cname;
    while(*cval && *cval != '=')
      cval++;
    if(*cval) {
      *cval = 0;
      snprintf(name_buf, sizeof(name_buf) - 1, "Cookie.%s", cname);
      name_buf[sizeof(name_buf) - 1] = 0;       // force null-terminate

      cval++;
      trailer = cval;
      while(*trailer && *trailer != ';')
        trailer++;
      if(*trailer) {
        cname = trailer + 1;
        *trailer = 0;
      } else {
        cname = trailer;
      }
      //hdf_set_value(cdata[idx].cgi->hdf, name_buf, cval);
      debug(DBG_GLOBAL, 5, "Setting cookie var %s to value '%s'",
            name_buf, cval);
    }
  }

}

int
isblank_x(char c)
{
  return (c == ' ' || c == '\r' || c == '\n' || c == '\t');
}

/* 
 * get the uri from already parsed dbuf, uri starts at "start", 
 * allocate the string and put it into the cdata structure
 */
int
http_parse_headers(int idx, int start, int old_dsize)
{
  int i;
  int j;
  char *cookies = NULL;
  char *http_host = NULL;
  appdata_http_t *ad = get_appdata(idx);
  dbuf_t *dbuf;

  if(ad == NULL) {
    debug(DBG_GLOBAL, 0, "Null appdata for HTTP, idx: %d", idx);
    return 0;
  }

  dbuf = ad->rdb;
  if(dbuf == NULL) {
    debug(DBG_GLOBAL, 0, "Null rdb for HTTP, idx: %d", idx);
    return 0;
  }

  i = dmemscan(dbuf, ' ', start);


  if(i < dbuf->dsize) {
    char *pc = alloca(dbuf->dsize + 1);

    if(pc == NULL) {
      debug(DBG_GLOBAL, 1,
            "Index %d could not alloca %d bytes for headers", idx,
            dbuf->dsize + 1);
      return 0;
    }
    // dbuf is already null-terminated!
    strcpy(pc, (char *) dbuf->buf);



    j = dmemscan(dbuf, '?', start);     // j is before i
    if(j < i) {
      ad->http_querystring = malloc(i - j);
      memcpy(ad->http_querystring, &pc[j + 1], i - j - 1);
      ad->http_querystring[i - j - 1] = 0;      // terminate the string
      setenv("QUERY_STRING", ad->http_querystring, 1);
    } else {
      j = i;
      ad->http_querystring = NULL;
      setenv("QUERY_STRING", "", 1);
    }
    // copy everything up to '?'
    ad->http_path = malloc(j - start + 1);
    memcpy(ad->http_path, &pc[start], j - start);
    ad->http_path[j - start] = 0;       // terminate the string

    j = i + 1;                  // Next char after space
    while(i < dbuf->dsize && tolower(pc[i]) != 'h' && pc[i] != '\r'
          && pc[i] != '\n')
      i++;
    if(i >= dbuf->dsize) {
      debug(DBG_GLOBAL, 1,
            "Index %d could not parse HTTP protocol - could not find 'h'",
            idx);
      return 0;
    }
    if((char *) strstr(&pc[j], "HTTP/1.") != &pc[j]) {
      debug(DBG_GLOBAL, 1,
            "Index %d could not parse HTTP protocol - found 'h' but not the rest",
            idx);
      return 0;
    }
    j = j + 7;                  //strlen("http/1.");
    if(pc[j] == '1') {
      ad->http_11 = 1;
    }
    j++;
    // skip crlf and any other junk
    while(j < dbuf->dsize && isspace(pc[j]))
      j++;
    // Supposedly we are in the beginning of the headers - stick them all into hashmap
    /*
       cdata[idx].http_headers = hashmap_create(128);
       if (cdata[idx].http_headers == 0) {
       debug(DBG_GLOBAL, 1, "Index %d could not create hashmap", idx);
       return 0;
       }
     */
    while(j < dbuf->dsize) {
      int k;
      int m;
      int n;

      k = j;
      // k is now the start of the header name, skip till colon and convert to lowercase
      while(k < dbuf->dsize && pc[k] != ':') {
        pc[k] = tolower(pc[k]);
        k++;
      }
      if(k < dbuf->dsize) {
        if(pc[k] == ':') {
          // Null-terminate. Now "j" is pointing to the lowercased header name
          pc[k] = 0;
          k++;
          // skip the blanks
          while(k < dbuf->dsize && pc[k] && isblank_x(pc[k]))
            k++;
          // k is now pointing to the start of the value
          m = k;
          do {
            // Skip till the end of the line
            while(m < dbuf->dsize && pc[m] != '\r' && pc[m] != '\n')
              m++;
            // Skip the end of line and check if the next line is multiline or not - snoop past the end of line
            // till the first char on the next line
            n = m;
            while(n < dbuf->dsize && (pc[n] == '\r' || pc[n] == '\n'))
              n++;
            if(n < dbuf->dsize && pc[n] == ' ') {
              m = n;
            } else {
              // null-terminate the header
              pc[m] = 0;
            }
          }
          while(n < dbuf->dsize && pc[n] == ' ');
          // TODO: need to fix the multiline headers properly (is there even such a thing ???)
          if(m < dbuf->dsize) {
            char *key = &pc[j];
            char *val = &pc[k];

            debug(DBG_GLOBAL, 1,
                  "index %d, header: '%s', value: '%s'", idx, &pc[j], &pc[k]);
            if(strcmp(key, "cookie") == 0) {
              cookies = val;
            }
            if(strcmp(key, "host") == 0) {
              http_host = val;
            }
            if(strcmp(key, "referer") == 0) {
              ad->http_referer = strdup(val);
            }
            if(strcmp(key, "content-type") == 0) {
              ad->content_type = strdup(val);
            }
            if(strcmp(key, "content-length") == 0) {
              ad->post_content_length = atoi(val);
              ad->post_content_got_length = 0;
              ad->post_content_buf = dalloc(ad->post_content_length);
            }
           
            //hashmap_insert(cdata[idx].http_headers, key, strlen(key)+1, val, strlen(val)+1);
            j = n;              // j is the beginning of the next header
          } else {
            debug(DBG_GLOBAL, 1, "moved past the end of line");
            return 0;
          }
        } else {
          // some garbage.. 
          debug(DBG_GLOBAL, 1, "moved past the end of line");
          return 0;
        }
      } else {
        j = k;
      }
    }

    if(ad->post_content_length > 0) {
      debug(DBG_GLOBAL, 1, "need to get the POST content: %d bytes", ad->post_content_length);
      int start_idx = dbuf->dsize+2;
      int body_index = start_idx;
      while(body_index < old_dsize && ad->post_content_got_length < ad->post_content_length) {
        // FIXME: this is not terribly efficient...
        body_index++;
        ad->post_content_got_length++;
      }
      if (body_index > start_idx) {
        dstrcat(ad->post_content_buf, (void *)&dbuf->buf[start_idx], body_index-start_idx);
      }
      if (ad->post_content_got_length == ad->post_content_length) {
        debug(DBG_GLOBAL, 1, "Got the POST content: '%s' (%d bytes / %d)", ad->post_content_buf->buf, ad->post_content_got_length, body_index-start_idx);
      }
    }

    //if (http_host)  hdf_set_value(cdata[idx].cgi->hdf, "HTTP.Host", http_host);


    return 1;
  } else {
    // somehow could not find the space - evil haxors ?
    debug(DBG_GLOBAL, 1, "index %d - could not parse uri !!!", idx);
    return 0;
  }
}

/* Check if the HTTP request for the idx is complete or not.
 * If it is, then the side effect is that the last crlfcrlf are wiped out 
 * and null-terminated.
 */
int
http_request_check(int idx)
{
  appdata_http_t *ad = get_appdata(idx);
  dbuf_t *dbuf = ad->rdb;
  int start_offs = 0;
  int offs;
  int old_dsize;

  while((offs = dmemscan(dbuf, '\n', start_offs)) <= dbuf->dsize - 2) {
    debug(DBG_GLOBAL, 40,
          "http_request_check: Index %d : found LF at %d", idx, offs);
    debug(DBG_GLOBAL, 45, "this char is: %d", dbuf->buf[offs]);
    debug(DBG_GLOBAL, 45, "next char is: %d", dbuf->buf[offs + 1]);


    if((dbuf->buf[offs + 1] == '\n') ||
       ((offs < dbuf->dsize - 2) && (dbuf->buf[offs + 1] == '\r')
        && (dbuf->buf[offs + 2] == '\n'))) {
      dbuf->buf[offs + 1] = 0;
      old_dsize = dbuf->dsize;
      dbuf->dsize = offs + 1;   /* zero terminator is there, but not accounted for! */
      debug(DBG_GLOBAL, 30, "request complete: '%s'!", dbuf->buf);
      /*
         Since the buf is now null-terminated, string ops are OK 
       */
      if(strstr((char *) dbuf->buf, "GET ")) {
        if(http_parse_headers(idx, 4, old_dsize)) {
          return HTTP_REQ_GET;
        }
      } else if(strstr((char *) dbuf->buf, "POST ")) {
        debug(DBG_GLOBAL, 45, "Probably a POST request, parsing headers...");
        if(http_parse_headers(idx, 5, old_dsize)) {
          return HTTP_REQ_POST;
        }
      } else if(strstr((char *) dbuf->buf, "CONNECT ")) {
        if(http_parse_headers(idx, 8, old_dsize)) {
          return HTTP_REQ_CONNECT;
        }
      }
      return HTTP_REQ_UNKNOWN;
    } else {
      /*
         apparently was a single LF, try to continue searching 
       */
      start_offs = offs + 1;
    }
  }
  return HTTP_REQ_INCOMPLETE;
}


/** 
 * Function to handle the http read
 */

int
ev_http_read(int idx, dbuf_t * d, void *u_ptr)
{
  appdata_http_t *ad = get_appdata(idx);

  if(ad == NULL) {
    debug(DBG_GLOBAL, 0, "Null appdata for HTTP, idx: %d", idx);
  }
  debug(DBG_GLOBAL, 1, "index %d idx, state: %d\n", idx, ad->l7state);
  switch (ad->l7state) {
  case HTTP_L7_INIT:
    if(ad->rdb == NULL) {
      // Since we are going to stick to use this block, lock it
      ad->rdb = dlock(d);
      debug(DBG_GLOBAL, 1,
            "Index %d : first buffering input data, still in L7_INIT ,dsize: %d",
            idx, ad->rdb->dsize);
    } else {
      if(!dconcat(ad->rdb, d)) {
        debug(DBG_GLOBAL, 1,
              "Index %d : could not concatenate the block!", idx);
      } else {
        debug(DBG_GLOBAL, 3,
              "Index %d : buffering input data, still in L7_INIT ,dsize: %d",
              idx, ad->rdb->dsize);
      }
    }
    switch (http_request_check(idx)) {
    case HTTP_REQ_UNKNOWN:
      debug(DBG_GLOBAL, 1, "Index %d got unknown request", idx);
      break;
    case HTTP_REQ_GET:
      debug(DBG_GLOBAL, 3,
            "Index %d got GET request for %s, moving to L7_SHOWTIME",
            idx, ad->http_path);
      ad->l7state = HTTP_L7_SHOWTIME;
      http_handle_request(idx);
      // Queued the data to send - reset to await the new stuff on the connection
      do_l7_reset(idx);
      break;
    case HTTP_REQ_POST:
      debug(DBG_GLOBAL, 3,
            "Index %d got POST request for %s..",
            idx, ad->http_path);
      if(ad->post_content_got_length == ad->post_content_length) {
        debug(DBG_GLOBAL, 3, "Got all the POST data (%d bytes), showtime!", ad->post_content_got_length);
        ad->l7state = HTTP_L7_SHOWTIME;
        http_handle_request(idx);
        // Queued the data to send - reset to await the new stuff on the connection
        do_l7_reset(idx);
      } else {
        debug(DBG_GLOBAL, 3, "Need more POST data, L7_READ_POST_DATA!");
        ad->l7state = HTTP_L7_READ_POST_DATA;
      }
      break;
    case HTTP_REQ_CONNECT:
      debug(DBG_GLOBAL, 3,
            "Index %d got CONNECT request for %s, moving to L7_SHOWTIME",
            idx, ad->http_path);
      ad->l7state = HTTP_L7_SHOWTIME;
      http_handle_request_connect(idx);
    default:
      debug(DBG_GLOBAL, 3,
            "Index %d could not determine request (yet?), still in L7_INIT",
            idx);
      debug(DBG_GLOBAL, 5, "dsize: %d",
            ad->rdb->dsize);

    }
    break;
  case HTTP_L7_READ_POST_DATA:
    debug(DBG_GLOBAL, 1, "Index %d, received data in L7_READ_POST_DATA - %d bytes !!!", idx, d->dsize);
    dstrcat(ad->post_content_buf, (void *)d->buf, d->dsize);
    ad->post_content_got_length += d->dsize;
    
    if(ad->post_content_got_length >= ad->post_content_length) {
      debug(DBG_GLOBAL, 3, "Got all the POST data (%d out of %d), showtime!", ad->post_content_got_length, ad->post_content_length);
      ad->l7state = HTTP_L7_SHOWTIME;
      http_handle_request(idx);
      // Queued the data to send - reset to await the new stuff on the connection
      do_l7_reset(idx);
    } else {
      debug(DBG_GLOBAL, 3, "Got some of the POST data (%d out of %d), need to read more...", ad->post_content_got_length, ad->post_content_length);
    }
    break;
  case HTTP_L7_SHOWTIME:
    debug(DBG_GLOBAL, 1, "Index %d, received data in SHOWTIME ???", idx);
    break;
  }
  return 1;

}

/**
 * read data from the HTTP-CONNECT socket
 */
int
ev_http_connect_read(int idx, dbuf_t * d, void *u_ptr)
{
  debug(DBG_GLOBAL, 5, "HTTP-CONNECT idx %d got %d bytes of data", idx,
        d->dsize);
  return 1;

}

