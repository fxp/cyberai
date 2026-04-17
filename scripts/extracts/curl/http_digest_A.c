/* ===== EXTRACT: http_digest.c [output+decode] ===== */
/* File: lib/http_digest.c — curl 8.11.0 */

/* Function: Curl_output_digest (L71–177) */
CURLcode Curl_output_digest(struct Curl_easy *data,
                            bool proxy,
                            const unsigned char *request,
                            const unsigned char *uripath)
{
  CURLcode result;
  unsigned char *path = NULL;
  char *tmp = NULL;
  char *response;
  size_t len;
  bool have_chlg;

  /* Point to the address of the pointer that holds the string to send to the
     server, which is for a plain host or for an HTTP proxy */
  char **allocuserpwd;

  /* Point to the name and password for this */
  const char *userp;
  const char *passwdp;

  /* Point to the correct struct with this */
  struct digestdata *digest;
  struct auth *authp;

  if(proxy) {
#ifdef CURL_DISABLE_PROXY
    return CURLE_NOT_BUILT_IN;
#else
    digest = &data->state.proxydigest;
    allocuserpwd = &data->state.aptr.proxyuserpwd;
    userp = data->state.aptr.proxyuser;
    passwdp = data->state.aptr.proxypasswd;
    authp = &data->state.authproxy;
#endif
  }
  else {
    digest = &data->state.digest;
    allocuserpwd = &data->state.aptr.userpwd;
    userp = data->state.aptr.user;
    passwdp = data->state.aptr.passwd;
    authp = &data->state.authhost;
  }

  Curl_safefree(*allocuserpwd);

  /* not set means empty */
  if(!userp)
    userp = "";

  if(!passwdp)
    passwdp = "";

#if defined(USE_WINDOWS_SSPI)
  have_chlg = !!digest->input_token;
#else
  have_chlg = !!digest->nonce;
#endif

  if(!have_chlg) {
    authp->done = FALSE;
    return CURLE_OK;
  }

  /* So IE browsers < v7 cut off the URI part at the query part when they
     evaluate the MD5 and some (IIS?) servers work with them so we may need to
     do the Digest IE-style. Note that the different ways cause different MD5
     sums to get sent.

     Apache servers can be set to do the Digest IE-style automatically using
     the BrowserMatch feature:
     https://httpd.apache.org/docs/2.2/mod/mod_auth_digest.html#msie

     Further details on Digest implementation differences:
     http://www.fngtps.com/2006/09/http-authentication
  */

  if(authp->iestyle) {
    tmp = strchr((char *)uripath, '?');
    if(tmp) {
      size_t urilen = tmp - (char *)uripath;
      /* typecast is fine here since the value is always less than 32 bits */
      path = (unsigned char *) aprintf("%.*s", (int)urilen, uripath);
    }
  }
  if(!tmp)
    path = (unsigned char *) strdup((char *) uripath);

  if(!path)
    return CURLE_OUT_OF_MEMORY;

  result = Curl_auth_create_digest_http_message(data, userp, passwdp, request,
                                                path, digest, &response, &len);
  free(path);
  if(result)
    return result;

  *allocuserpwd = aprintf("%sAuthorization: Digest %s\r\n",
                          proxy ? "Proxy-" : "",
                          response);
  free(response);
  if(!*allocuserpwd)
    return CURLE_OUT_OF_MEMORY;

  authp->done = TRUE;

  return CURLE_OK;
}

/* Function: Curl_auth_decode_digest_http_message (L68–75) */
  return Curl_auth_decode_digest_http_message(header, digest);
}

CURLcode Curl_output_digest(struct Curl_easy *data,
                            bool proxy,
                            const unsigned char *request,
                            const unsigned char *uripath)
{
