/* ===== EXTRACT: urlapi.c [curl_url_set+curl_url_get] ===== */
/* Lines: 1702–2015 */
/* File: lib/urlapi.c — curl 8.11.0 */

CURLUcode curl_url_set(CURLU *u, CURLUPart what,
                       const char *part, unsigned int flags)
{
  char **storep = NULL;
  bool urlencode = (flags & CURLU_URLENCODE) ? 1 : 0;
  bool plusencode = FALSE;
  bool urlskipslash = FALSE;
  bool leadingslash = FALSE;
  bool appendquery = FALSE;
  bool equalsencode = FALSE;
  size_t nalloc;

  if(!u)
    return CURLUE_BAD_HANDLE;
  if(!part) {
    /* setting a part to NULL clears it */
    switch(what) {
    case CURLUPART_URL:
      break;
    case CURLUPART_SCHEME:
      storep = &u->scheme;
      u->guessed_scheme = FALSE;
      break;
    case CURLUPART_USER:
      storep = &u->user;
      break;
    case CURLUPART_PASSWORD:
      storep = &u->password;
      break;
    case CURLUPART_OPTIONS:
      storep = &u->options;
      break;
    case CURLUPART_HOST:
      storep = &u->host;
      break;
    case CURLUPART_ZONEID:
      storep = &u->zoneid;
      break;
    case CURLUPART_PORT:
      u->portnum = 0;
      storep = &u->port;
      break;
    case CURLUPART_PATH:
      storep = &u->path;
      break;
    case CURLUPART_QUERY:
      storep = &u->query;
      u->query_present = FALSE;
      break;
    case CURLUPART_FRAGMENT:
      storep = &u->fragment;
      u->fragment_present = FALSE;
      break;
    default:
      return CURLUE_UNKNOWN_PART;
    }
    if(storep && *storep) {
      Curl_safefree(*storep);
    }
    else if(!storep) {
      free_urlhandle(u);
      memset(u, 0, sizeof(struct Curl_URL));
    }
    return CURLUE_OK;
  }

  nalloc = strlen(part);
  if(nalloc > CURL_MAX_INPUT_LENGTH)
    /* excessive input length */
    return CURLUE_MALFORMED_INPUT;

  switch(what) {
  case CURLUPART_SCHEME: {
    size_t plen = strlen(part);
    const char *s = part;
    if((plen > MAX_SCHEME_LEN) || (plen < 1))
      /* too long or too short */
      return CURLUE_BAD_SCHEME;
   /* verify that it is a fine scheme */
    if(!(flags & CURLU_NON_SUPPORT_SCHEME) && !Curl_get_scheme_handler(part))
      return CURLUE_UNSUPPORTED_SCHEME;
    storep = &u->scheme;
    urlencode = FALSE; /* never */
    if(ISALPHA(*s)) {
      /* ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ) */
      while(--plen) {
        if(ISALNUM(*s) || (*s == '+') || (*s == '-') || (*s == '.'))
          s++; /* fine */
        else
          return CURLUE_BAD_SCHEME;
      }
    }
    else
      return CURLUE_BAD_SCHEME;
    u->guessed_scheme = FALSE;
    break;
  }
  case CURLUPART_USER:
    storep = &u->user;
    break;
  case CURLUPART_PASSWORD:
    storep = &u->password;
    break;
  case CURLUPART_OPTIONS:
    storep = &u->options;
    break;
  case CURLUPART_HOST:
    storep = &u->host;
    Curl_safefree(u->zoneid);
    break;
  case CURLUPART_ZONEID:
    storep = &u->zoneid;
    break;
  case CURLUPART_PORT:
    if(!ISDIGIT(part[0]))
      /* not a number */
      return CURLUE_BAD_PORT_NUMBER;
    else {
      char *tmp;
      char *endp;
      unsigned long port;
      errno = 0;
      port = strtoul(part, &endp, 10);  /* must be decimal */
      if(errno || (port > 0xffff) || *endp)
        /* weirdly provided number, not good! */
        return CURLUE_BAD_PORT_NUMBER;
      tmp = strdup(part);
      if(!tmp)
        return CURLUE_OUT_OF_MEMORY;
      free(u->port);
      u->port = tmp;
      u->portnum = (unsigned short)port;
      return CURLUE_OK;
    }
  case CURLUPART_PATH:
    urlskipslash = TRUE;
    leadingslash = TRUE; /* enforce */
    storep = &u->path;
    break;
  case CURLUPART_QUERY:
    plusencode = urlencode;
    appendquery = (flags & CURLU_APPENDQUERY) ? 1 : 0;
    equalsencode = appendquery;
    storep = &u->query;
    u->query_present = TRUE;
    break;
  case CURLUPART_FRAGMENT:
    storep = &u->fragment;
    u->fragment_present = TRUE;
    break;
  case CURLUPART_URL: {
    /*
     * Allow a new URL to replace the existing (if any) contents.
     *
     * If the existing contents is enough for a URL, allow a relative URL to
     * replace it.
     */
    CURLcode result;
    CURLUcode uc;
    char *oldurl;
    char *redired_url;

    if(!nalloc)
      /* a blank URL is not a valid URL */
      return CURLUE_MALFORMED_INPUT;

    /* if the new thing is absolute or the old one is not
     * (we could not get an absolute URL in 'oldurl'),
     * then replace the existing with the new. */
    if(Curl_is_absolute_url(part, NULL, 0,
                            flags & (CURLU_GUESS_SCHEME|
                                     CURLU_DEFAULT_SCHEME))
       || curl_url_get(u, CURLUPART_URL, &oldurl, flags)) {
      return parseurl_and_replace(part, u, flags);
    }

    /* apply the relative part to create a new URL
     * and replace the existing one with it. */
    result = concat_url(oldurl, part, &redired_url);
    free(oldurl);
    if(result)
      return cc2cu(result);

    uc = parseurl_and_replace(redired_url, u, flags);
    free(redired_url);
    return uc;
  }
  default:
    return CURLUE_UNKNOWN_PART;
  }
  DEBUGASSERT(storep);
  {
    const char *newp;
    struct dynbuf enc;
    Curl_dyn_init(&enc, nalloc * 3 + 1 + leadingslash);

    if(leadingslash && (part[0] != '/')) {
      CURLcode result = Curl_dyn_addn(&enc, "/", 1);
      if(result)
        return cc2cu(result);
    }
    if(urlencode) {
      const unsigned char *i;

      for(i = (const unsigned char *)part; *i; i++) {
        CURLcode result;
        if((*i == ' ') && plusencode) {
          result = Curl_dyn_addn(&enc, "+", 1);
          if(result)
            return CURLUE_OUT_OF_MEMORY;
        }
        else if(ISUNRESERVED(*i) ||
                ((*i == '/') && urlskipslash) ||
                ((*i == '=') && equalsencode)) {
          if((*i == '=') && equalsencode)
            /* only skip the first equals sign */
            equalsencode = FALSE;
          result = Curl_dyn_addn(&enc, i, 1);
          if(result)
            return cc2cu(result);
        }
        else {
          char out[3]={'%'};
          out[1] = hexdigits[*i >> 4];
          out[2] = hexdigits[*i & 0xf];
          result = Curl_dyn_addn(&enc, out, 3);
          if(result)
            return cc2cu(result);
        }
      }
    }
    else {
      char *p;
      CURLcode result = Curl_dyn_add(&enc, part);
      if(result)
        return cc2cu(result);
      p = Curl_dyn_ptr(&enc);
      while(*p) {
        /* make sure percent encoded are lower case */
        if((*p == '%') && ISXDIGIT(p[1]) && ISXDIGIT(p[2]) &&
           (ISUPPER(p[1]) || ISUPPER(p[2]))) {
          p[1] = Curl_raw_tolower(p[1]);
          p[2] = Curl_raw_tolower(p[2]);
          p += 3;
        }
        else
          p++;
      }
    }
    newp = Curl_dyn_ptr(&enc);

    if(appendquery && newp) {
      /* Append the 'newp' string onto the old query. Add a '&' separator if
         none is present at the end of the existing query already */

      size_t querylen = u->q