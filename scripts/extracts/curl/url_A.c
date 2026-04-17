/* ===== EXTRACT: url.c [parseurlandfillconn+setup_range] ===== */
/* File: lib/url.c — curl 8.11.0 */

/* Function: parseurlandfillconn (L1728–1941) */
static CURLcode parseurlandfillconn(struct Curl_easy *data,
                                    struct connectdata *conn)
{
  CURLcode result;
  CURLU *uh;
  CURLUcode uc;
  char *hostname;
  bool use_set_uh = (data->set.uh && !data->state.this_is_a_follow);

  up_free(data); /* cleanup previous leftovers first */

  /* parse the URL */
  if(use_set_uh) {
    uh = data->state.uh = curl_url_dup(data->set.uh);
  }
  else {
    uh = data->state.uh = curl_url();
  }

  if(!uh)
    return CURLE_OUT_OF_MEMORY;

  if(data->set.str[STRING_DEFAULT_PROTOCOL] &&
     !Curl_is_absolute_url(data->state.url, NULL, 0, TRUE)) {
    char *url = aprintf("%s://%s", data->set.str[STRING_DEFAULT_PROTOCOL],
                        data->state.url);
    if(!url)
      return CURLE_OUT_OF_MEMORY;
    if(data->state.url_alloc)
      free(data->state.url);
    data->state.url = url;
    data->state.url_alloc = TRUE;
  }

  if(!use_set_uh) {
    char *newurl;
    uc = curl_url_set(uh, CURLUPART_URL, data->state.url, (unsigned int)
                      (CURLU_GUESS_SCHEME |
                       CURLU_NON_SUPPORT_SCHEME |
                       (data->set.disallow_username_in_url ?
                        CURLU_DISALLOW_USER : 0) |
                       (data->set.path_as_is ? CURLU_PATH_AS_IS : 0)));
    if(uc) {
      failf(data, "URL rejected: %s", curl_url_strerror(uc));
      return Curl_uc_to_curlcode(uc);
    }

    /* after it was parsed, get the generated normalized version */
    uc = curl_url_get(uh, CURLUPART_URL, &newurl, 0);
    if(uc)
      return Curl_uc_to_curlcode(uc);
    if(data->state.url_alloc)
      free(data->state.url);
    data->state.url = newurl;
    data->state.url_alloc = TRUE;
  }

  uc = curl_url_get(uh, CURLUPART_SCHEME, &data->state.up.scheme, 0);
  if(uc)
    return Curl_uc_to_curlcode(uc);

  uc = curl_url_get(uh, CURLUPART_HOST, &data->state.up.hostname, 0);
  if(uc) {
    if(!strcasecompare("file", data->state.up.scheme))
      return CURLE_OUT_OF_MEMORY;
  }
  else if(strlen(data->state.up.hostname) > MAX_URL_LEN) {
    failf(data, "Too long hostname (maximum is %d)", MAX_URL_LEN);
    return CURLE_URL_MALFORMAT;
  }
  hostname = data->state.up.hostname;

  if(hostname && hostname[0] == '[') {
    /* This looks like an IPv6 address literal. See if there is an address
       scope. */
    size_t hlen;
    conn->bits.ipv6_ip = TRUE;
    /* cut off the brackets! */
    hostname++;
    hlen = strlen(hostname);
    hostname[hlen - 1] = 0;

    zonefrom_url(uh, data, conn);
  }

  /* make sure the connect struct gets its own copy of the hostname */
  conn->host.rawalloc = strdup(hostname ? hostname : "");
  if(!conn->host.rawalloc)
    return CURLE_OUT_OF_MEMORY;
  conn->host.name = conn->host.rawalloc;

  /*************************************************************
   * IDN-convert the hostnames
   *************************************************************/
  result = Curl_idnconvert_hostname(&conn->host);
  if(result)
    return result;

#ifndef CURL_DISABLE_HSTS
  /* HSTS upgrade */
  if(data->hsts && strcasecompare("http", data->state.up.scheme)) {
    /* This MUST use the IDN decoded name */
    if(Curl_hsts(data->hsts, conn->host.name, TRUE)) {
      char *url;
      Curl_safefree(data->state.up.scheme);
      uc = curl_url_set(uh, CURLUPART_SCHEME, "https", 0);
      if(uc)
        return Curl_uc_to_curlcode(uc);
      if(data->state.url_alloc)
        Curl_safefree(data->state.url);
      /* after update, get the updated version */
      uc = curl_url_get(uh, CURLUPART_URL, &url, 0);
      if(uc)
        return Curl_uc_to_curlcode(uc);
      uc = curl_url_get(uh, CURLUPART_SCHEME, &data->state.up.scheme, 0);
      if(uc) {
        free(url);
        return Curl_uc_to_curlcode(uc);
      }
      data->state.url = url;
      data->state.url_alloc = TRUE;
      infof(data, "Switched from HTTP to HTTPS due to HSTS => %s",
            data->state.url);
    }
  }
#endif

  result = findprotocol(data, conn, data->state.up.scheme);
  if(result)
    return result;

  /*
   * username and password set with their own options override the credentials
   * possibly set in the URL, but netrc does not.
   */
  if(!data->state.aptr.passwd || (data->state.creds_from != CREDS_OPTION)) {
    uc = curl_url_get(uh, CURLUPART_PASSWORD, &data->state.up.password, 0);
    if(!uc) {
      char *decoded;
      result = Curl_urldecode(data->state.up.password, 0, &decoded, NULL,
                              conn->handler->flags&PROTOPT_USERPWDCTRL ?
                              REJECT_ZERO : REJECT_CTRL);
      if(result)
        return result;
      conn->passwd = decoded;
      result = Curl_setstropt(&data->state.aptr.passwd, decoded);
      if(result)
        return result;
      data->state.creds_from = CREDS_URL;
    }
    else if(uc != CURLUE_NO_PASSWORD)
      return Curl_uc_to_curlcode(uc);
  }

  if(!data->state.aptr.user || (data->state.creds_from != CREDS_OPTION)) {
    /* we do not use the URL API's URL decoder option here since it rejects
       control codes and we want to allow them for some schemes in the user
       and password fields */
    uc = curl_url_get(uh, CURLUPART_USER, &data->state.up.user, 0);
    if(!uc) {
      char *decoded;
      result = Curl_urldecode(data->state.up.user, 0, &decoded, NULL,
                              conn->handler->flags&PROTOPT_USERPWDCTRL ?
                              REJECT_ZERO : REJECT_CTRL);
      if(result)
        return result;
      conn->user = decoded;
      result = Curl_setstropt(&data->state.aptr.user, decoded);
      data->state.creds_from = CREDS_URL;
    }
    else if(uc != CURLUE_NO_USER)
      return Curl_uc_to_curlcode(uc);
    if(result)
      return result;
  }

  uc = curl_url_get(uh, CURLUPART_OPTIONS, &data->state.up.options,
                    CURLU_URLDECODE);
  if(!uc) {
    conn->options = strdup(data->state.up.options);
    if(!conn->options)
      return CURLE_OUT_OF_MEMORY;
  }
  else if(uc != CURLUE_NO_OPTIONS)
    return Curl_uc_to_curlcode(uc);

  uc = curl_url_get(uh, CURLUPART_PATH, &data->state.up.path,
                    CURLU_URLENCODE);
  if(uc)
    return Curl_uc_to_curlcode(uc);

  uc = curl_url_get(uh, CURLUPART_PORT, &data->state.up.port,
                    CURLU_DEFAULT_PORT);
  if(uc) {
    if(!strcasecompare("file", data->state.up.scheme))
      return CURLE_OUT_OF_MEMORY;
  }
  else {
    unsigned long port = strtoul(data->state.up.port, NULL, 10);
    conn->primary.remote_port = conn->remote_port =
      (data->set.use_port && data->state.allow_port) ?
      data->set.use_port : curlx_ultous(port);
  }

  (void)curl_url_get(uh, CURLUPART_QUERY, &data->state.up.query, 0);

#ifdef USE_IPV6
  if(data->set.scope_id)
    /* Override any scope that was set above.  */
    conn->scope_id = data->set.scope_id;
#endif

  return CURLE_OK;
}

/* Function: setup_range (L1948–1973) */
static CURLcode setup_range(struct Curl_easy *data)
{
  struct UrlState *s = &data->state;
  s->resume_from = data->set.set_resume_from;
  if(s->resume_from || data->set.str[STRING_SET_RANGE]) {
    if(s->rangestringalloc)
      free(s->range);

    if(s->resume_from)
      s->range = aprintf("%" FMT_OFF_T "-", s->resume_from);
    else
      s->range = strdup(data->set.str[STRING_SET_RANGE]);

    if(!s->range)
      return CURLE_OUT_OF