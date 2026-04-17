/* proxy: hop-by-hop header filter init */

/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#define  NGX_HTTP_PROXY_COOKIE_SECURE           0x0001
#define  NGX_HTTP_PROXY_COOKIE_SECURE_ON        0x0002
#define  NGX_HTTP_PROXY_COOKIE_SECURE_OFF       0x0004
#define  NGX_HTTP_PROXY_COOKIE_HTTPONLY         0x0008
#define  NGX_HTTP_PROXY_COOKIE_HTTPONLY_ON      0x0010
#define  NGX_HTTP_PROXY_COOKIE_HTTPONLY_OFF     0x0020
#define  NGX_HTTP_PROXY_COOKIE_SAMESITE         0x0040
#define  NGX_HTTP_PROXY_COOKIE_SAMESITE_STRICT  0x0080
#define  NGX_HTTP_PROXY_COOKIE_SAMESITE_LAX     0x0100
#define  NGX_HTTP_PROXY_COOKIE_SAMESITE_NONE    0x0200
#define  NGX_HTTP_PROXY_COOKIE_SAMESITE_OFF     0x0400


typedef struct {
    ngx_array_t                    caches;  /* ngx_http_file_cache_t * */
} ngx_http_proxy_main_conf_t;


typedef struct ngx_http_proxy_rewrite_s  ngx_http_proxy_rewrite_t;

typedef ngx_int_t (*ngx_http_proxy_rewrite_pt)(ngx_http_request_t *r,
    ngx_str_t *value, size_t prefix, size_t len,
    ngx_http_proxy_rewrite_t *pr);

struct ngx_http_proxy_rewrite_s {
    ngx_http_proxy_rewrite_pt      handler;

    union {
        ngx_http_complex_value_t   complex;
#if (NGX_PCRE)
        ngx_http_regex_t          *regex;
#endif
    } pattern;

    ngx_http_complex_value_t       replacement;
};


typedef struct {
    union {
        ngx_http_complex_value_t   complex;
#if (NGX_PCRE)
        ngx_http_regex_t          *regex;
#endif
    } cookie;

    ngx_array_t                    flags_values;
    ngx_uint_t                     regex;
} ngx_http_proxy_cookie_flags_t;


typedef struct {
    ngx_str_t                      key_start;
    ngx_str_t                      schema;
    ngx_str_t                      host_header;
    ngx_str_t                      port;
    ngx_str_t                      uri;
} ngx_http_proxy_vars_t;


typedef struct {
    ngx_array_t                   *flushes;
    ngx_array_t                   *lengths;
    ngx_array_t                   *values;
    ngx_hash_t                     hash;
} ngx_http_proxy_headers_t;


typedef struct {
    ngx_http_upstream_conf_t       upstream;

    ngx_array_t                   *body_flushes;
    ngx_array_t                   *body_lengths;
    ngx_array_t                   *body_values;
    ngx_str_t                      body_source;

    ngx_http_proxy_headers_t       headers;
#if (NGX_HTTP_CACHE)
    ngx_http_proxy_headers_t       headers_cache;
#endif
    ngx_array_t                   *headers_source;

    ngx_array_t                   *proxy_lengths;
    ngx_array_t                   *proxy_values;

    ngx_array_t                   *redirects;
    ngx_array_t                   *cookie_domains;
    ngx_array_t                   *cookie_paths;
