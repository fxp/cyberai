/* ===== EXTRACT: socks.c [do_SOCKS4] ===== */
/* Lines: 1–350 */
/* File: lib/socks.c — curl 8.11.0 */

/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/

#include "curl_setup.h"

#if !defined(CURL_DISABLE_PROXY)

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "urldata.h"
#include "sendf.h"
#include "select.h"
#include "cfilters.h"
#include "connect.h"
#include "timeval.h"
#include "socks.h"
#include "multiif.h" /* for getsock macros */
#include "inet_pton.h"
#include "url.h"

/* The last 3 #include files should be in this order */
#include "curl_printf.h"
#include "curl_memory.h"
#include "memdebug.h"

/* for the (SOCKS) connect state machine */
enum connect_t {
  CONNECT_INIT,
  CONNECT_SOCKS_INIT, /* 1 */
  CONNECT_SOCKS_SEND, /* 2 waiting to send more first data */
  CONNECT_SOCKS_READ_INIT, /* 3 set up read */
  CONNECT_SOCKS_READ, /* 4 read server response */
  CONNECT_GSSAPI_INIT, /* 5 */
  CONNECT_AUTH_INIT, /* 6 setup outgoing auth buffer */
  CONNECT_AUTH_SEND, /* 7 send auth */
  CONNECT_AUTH_READ, /* 8 read auth response */
  CONNECT_REQ_INIT,  /* 9 init SOCKS "request" */
  CONNECT_RESOLVING, /* 10 */
  CONNECT_RESOLVED,  /* 11 */
  CONNECT_RESOLVE_REMOTE, /* 12 */
  CONNECT_REQ_SEND,  /* 13 */
  CONNECT_REQ_SENDING, /* 14 */
  CONNECT_REQ_READ,  /* 15 */
  CONNECT_REQ_READ_MORE, /* 16 */
  CONNECT_DONE /* 17 connected fine to the remote or the SOCKS proxy */
};

#define CURL_SOCKS_BUF_SIZE 600

/* make sure we configure it not too low */
#if CURL_SOCKS_BUF_SIZE < 600
#error CURL_SOCKS_BUF_SIZE must be at least 600
#endif


struct socks_state {
  enum connect_t state;
  ssize_t outstanding;  /* send this many bytes more */
  unsigned char buffer[CURL_SOCKS_BUF_SIZE];
  unsigned char *outp; /* send from this pointer */

  const char *hostname;
  int remote_port;
  const char *proxy_user;
  const char *proxy_password;
};

#if defined(HAVE_GSSAPI) || defined(USE_WINDOWS_SSPI)
/*
 * Helper read-from-socket functions. Does the same as Curl_read() but it
 * blocks until all bytes amount of buffersize will be read. No more, no less.
 *
 * This is STUPID BLOCKING behavior. Only used by the SOCKS GSSAPI functions.
 */
int Curl_blockread_all(struct Curl_cfilter *cf,
                       struct Curl_easy *data,   /* transfer */
                       char *buf,                /* store read data here */
                       ssize_t buffersize,       /* max amount to read */
                       ssize_t *n)               /* amount bytes read */
{
  ssize_t nread = 0;
  ssize_t allread = 0;
  int result;
  CURLcode err = CURLE_OK;

  *n = 0;
  for(;;) {
    timediff_t timeout_ms = Curl_timeleft(data, NULL, TRUE);
    if(timeout_ms < 0) {
      /* we already got the timeout */
      result = CURLE_OPERATION_TIMEDOUT;
      break;
    }
    if(!timeout_ms)
      timeout_ms = TIMEDIFF_T_MAX;
    if(SOCKET_READABLE(cf->conn->sock[cf->sockindex], timeout_ms) <= 0) {
      result = ~CURLE_OK;
      break;
    }
    nread = Curl_conn_cf_recv(cf->next, data, buf, buffersize, &err);
    if(nread <= 0) {
      result = (int)err;
      if(CURLE_AGAIN == err)
        continue;
      if(err) {
        break;
      }
    }

    if(buffersize == nread) {
      allread += nread;
      *n = allread;
      result = CURLE_OK;
      break;
    }
    if(!nread) {
      result = ~CURLE_OK;
      break;
    }

    buffersize -= nread;
    buf += nread;
    allread += nread;
  }
  return result;
}
#endif

#if defined(DEBUGBUILD) && !defined(CURL_DISABLE_VERBOSE_STRINGS)
#define DEBUG_AND_VERBOSE
#define sxstate(x,d,y) socksstate(x,d,y, __LINE__)
#else
#define sxstate(x,d,y) socksstate(x,d,y)
#endif

/* always use this function to change state, to make debugging easier */
static void socksstate(struct socks_state *sx, struct Curl_easy *data,
                       enum connect_t state
#ifdef DEBUG_AND_VERBOSE
                       , int lineno
#endif
)
{
  enum connect_t oldstate = sx->state;
#ifdef DEBUG_AND_VERBOSE
  /* synced with the state list in urldata.h */
  static const char * const socks_statename[] = {
    "INIT",
    "SOCKS_INIT",
    "SOCKS_SEND",
    "SOCKS_READ_INIT",
    "SOCKS_READ",
    "GSSAPI_INIT",
    "AUTH_INIT",
    "AUTH_SEND",
    "AUTH_READ",
    "REQ_INIT",
    "RESOLVING",
    "RESOLVED",
    "RESOLVE_REMOTE",
    "REQ_SEND",
    "REQ_SENDING",
    "REQ_READ",
    "REQ_READ_MORE",
    "DONE"
  };
#endif

  (void)data;
  if(oldstate == state)
    /* do not bother when the new state is the same as the old state */
    return;

  sx->state = state;

#ifdef DEBUG_AND_VERBOSE
  infof(data,
        "SXSTATE: %s => %s; line %d",
        socks_statename[oldstate], socks_statename[sx->state],
        lineno);
#endif
}

static CURLproxycode socks_state_send(struct Curl_cfilter *cf,
                                      struct socks_state *sx,
                                      struct Curl_easy *data,
                                      CURLproxycode failcode,
                                      const char *description)
{
  ssize_t nwritten;
  CURLcode result;

  nwritten = Curl_conn_cf_send(cf->next, data, (char *)sx->outp,
                               sx->outstanding, FALSE, &result);
  if(nwritten <= 0) {
    if(CURLE_AGAIN == result) {
      return CURLPX_OK;
    }
    else if(CURLE_OK == result) {
      /* connection closed */
      failf(data, "connection to proxy closed");
      return CURLPX_CLOSED;
    }
    failf(data, "Failed to send %s: %s", description,
          curl_easy_strerror(result));
    return failcode;
  }
  DEBUGASSERT(sx->outstanding >= nwritten);
  /* not done, remain in state */
  sx->outstanding -= nwritten;
  sx->outp += nwritten;
  return CURLPX_OK;
}

static CURLproxycode socks_state_recv(struct Curl_cfilter *cf,
                                      struct socks_state *sx,
                                      struct Curl_easy *data,
                                      CURLproxyc