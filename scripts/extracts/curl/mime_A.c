/* ===== EXTRACT: mime.c [mime_read+mime_size+boundary] ===== */
/* Lines: 1–400 */
/* File: lib/mime.c — curl 8.11.0 */

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

#include <curl/curl.h>

struct Curl_easy;

#include "mime.h"
#include "warnless.h"
#include "urldata.h"
#include "sendf.h"
#include "strdup.h"

#if !defined(CURL_DISABLE_MIME) && (!defined(CURL_DISABLE_HTTP) ||      \
                                    !defined(CURL_DISABLE_SMTP) ||      \
                                    !defined(CURL_DISABLE_IMAP))

#if defined(HAVE_LIBGEN_H) && defined(HAVE_BASENAME)
#include <libgen.h>
#endif

#include "rand.h"
#include "slist.h"
#include "strcase.h"
#include "dynbuf.h"
/* The last 3 #include files should be in this order */
#include "curl_printf.h"
#include "curl_memory.h"
#include "memdebug.h"

#ifdef _WIN32
# ifndef R_OK
#  define R_OK 4
# endif
#endif


#define READ_ERROR                      ((size_t) -1)
#define STOP_FILLING                    ((size_t) -2)

static size_t mime_subparts_read(char *buffer, size_t size, size_t nitems,
                                 void *instream, bool *hasread);

/* Encoders. */
static size_t encoder_nop_read(char *buffer, size_t size, bool ateof,
                                curl_mimepart *part);
static curl_off_t encoder_nop_size(curl_mimepart *part);
static size_t encoder_7bit_read(char *buffer, size_t size, bool ateof,
                                curl_mimepart *part);
static size_t encoder_base64_read(char *buffer, size_t size, bool ateof,
                                curl_mimepart *part);
static curl_off_t encoder_base64_size(curl_mimepart *part);
static size_t encoder_qp_read(char *buffer, size_t size, bool ateof,
                              curl_mimepart *part);
static curl_off_t encoder_qp_size(curl_mimepart *part);
static curl_off_t mime_size(curl_mimepart *part);

static const struct mime_encoder encoders[] = {
  {"binary", encoder_nop_read, encoder_nop_size},
  {"8bit", encoder_nop_read, encoder_nop_size},
  {"7bit", encoder_7bit_read, encoder_nop_size},
  {"base64", encoder_base64_read, encoder_base64_size},
  {"quoted-printable", encoder_qp_read, encoder_qp_size},
  {ZERO_NULL, ZERO_NULL, ZERO_NULL}
};

/* Base64 encoding table */
static const char base64enc[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Quoted-printable character class table.
 *
 * We cannot rely on ctype functions since quoted-printable input data
 * is assumed to be ASCII-compatible, even on non-ASCII platforms. */
#define QP_OK           1       /* Can be represented by itself. */
#define QP_SP           2       /* Space or tab. */
#define QP_CR           3       /* Carriage return. */
#define QP_LF           4       /* Line-feed. */
static const unsigned char qp_class[] = {
 0,     0,     0,     0,     0,     0,     0,     0,            /* 00 - 07 */
 0,     QP_SP, QP_LF, 0,     0,     QP_CR, 0,     0,            /* 08 - 0F */
 0,     0,     0,     0,     0,     0,     0,     0,            /* 10 - 17 */
 0,     0,     0,     0,     0,     0,     0,     0,            /* 18 - 1F */
 QP_SP, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK,        /* 20 - 27 */
 QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK,        /* 28 - 2F */
 QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK,        /* 30 - 37 */
 QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, 0    , QP_OK, QP_OK,        /* 38 - 3F */
 QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK,        /* 40 - 47 */
 QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK,        /* 48 - 4F */
 QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK,        /* 50 - 57 */
 QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK,        /* 58 - 5F */
 QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK,        /* 60 - 67 */
 QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK,        /* 68 - 6F */
 QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK,        /* 70 - 77 */
 QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, QP_OK, 0,            /* 78 - 7F */
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                /* 80 - 8F */
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                /* 90 - 9F */
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                /* A0 - AF */
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                /* B0 - BF */
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                /* C0 - CF */
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                /* D0 - DF */
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                /* E0 - EF */
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                 /* F0 - FF */
};


/* Binary --> hexadecimal ASCII table. */
static const char aschex[] =
  "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x41\x42\x43\x44\x45\x46";



#ifndef __VMS
#define filesize(name, stat_data) (stat_data.st_size)
#define fopen_read fopen

#else

#include <fabdef.h>
/*
 * get_vms_file_size does what it takes to get the real size of the file
 *
 * For fixed files, find out the size of the EOF block and adjust.
 *
 * For all others, have to read the entire file in, discarding the contents.
 * Most posted text files will be small, and binary files like zlib archives
 * and CD/DVD images should be either a STREAM_LF format or a fixed format.
 *
 */
curl_off_t VmsRealFileSize(const char *name,
                           const struct_stat *stat_buf)
{
  char buffer[8192];
  curl_off_t count;
  int ret_stat;
  FILE * file;

  file = fopen(name, FOPEN_READTEXT); /* VMS */
  if(!file)
    return 0;

  count = 0;
  ret_stat = 1;
  while(ret_stat > 0) {
    ret_stat = fread(buffer, 1, sizeof(buffer), file);
    if(ret_stat)
      count += ret_stat;
  }
  fclose(file);

  return count;
}

/*
 *
 *  VmsSpecialSize checks to see if the stat st_size can be trusted and
 *  if not to call a routine to get the correct size.
 *
 */
static curl_off_t VmsSpecialSize(const char *name,
                     