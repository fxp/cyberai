/* ===== EXTRACT: cookie.c [parse_cookie_header+sanitize+add] ===== */
/* File: lib/cookie.c — curl 8.11.0 */

/* Function: parse_cookie_header (L473–795) */
parse_cookie_header(struct Curl_easy *data,
                    struct Cookie *co,
                    struct CookieInfo *ci,
                    const char *ptr,
                    const char *domain, /* default domain */
                    const char *path,   /* full path used when this cookie is
                                           set, used to get default path for
                                           the cookie unless set */
                    bool secure)  /* TRUE if connection is over secure
                                     origin */
{
  /* This line was read off an HTTP-header */
  time_t now;
  size_t linelength = strlen(ptr);
  if(linelength > MAX_COOKIE_LINE)
    /* discard overly long lines at once */
    return CERR_TOO_LONG;

  now = time(NULL);
  do {
    size_t vlen;
    size_t nlen;

    while(*ptr && ISBLANK(*ptr))
      ptr++;

    /* we have a <name>=<value> pair or a stand-alone word here */
    nlen = strcspn(ptr, ";\t\r\n=");
    if(nlen) {
      bool done = FALSE;
      bool sep = FALSE;
      const char *namep = ptr;
      const char *valuep;

      ptr += nlen;

      /* trim trailing spaces and tabs after name */
      while(nlen && ISBLANK(namep[nlen - 1]))
        nlen--;

      if(*ptr == '=') {
        vlen = strcspn(++ptr, ";\r\n");
        valuep = ptr;
        sep = TRUE;
        ptr = &valuep[vlen];

        /* Strip off trailing whitespace from the value */
        while(vlen && ISBLANK(valuep[vlen-1]))
          vlen--;

        /* Skip leading whitespace from the value */
        while(vlen && ISBLANK(*valuep)) {
          valuep++;
          vlen--;
        }

        /* Reject cookies with a TAB inside the value */
        if(memchr(valuep, '\t', vlen)) {
          infof(data, "cookie contains TAB, dropping");
          return CERR_TAB;
        }
      }
      else {
        valuep = NULL;
        vlen = 0;
      }

      /*
       * Check for too long individual name or contents, or too long
       * combination of name + contents. Chrome and Firefox support 4095 or
       * 4096 bytes combo
       */
      if(nlen >= (MAX_NAME-1) || vlen >= (MAX_NAME-1) ||
         ((nlen + vlen) > MAX_NAME)) {
        infof(data, "oversized cookie dropped, name/val %zu + %zu bytes",
              nlen, vlen);
        return CERR_TOO_BIG;
      }

      /*
       * Check if we have a reserved prefix set before anything else, as we
       * otherwise have to test for the prefix in both the cookie name and
       * "the rest". Prefixes must start with '__' and end with a '-', so
       * only test for names where that can possibly be true.
       */
      if(nlen >= 7 && namep[0] == '_' && namep[1] == '_') {
        if(strncasecompare("__Secure-", namep, 9))
          co->prefix_secure = TRUE;
        else if(strncasecompare("__Host-", namep, 7))
          co->prefix_host = TRUE;
      }

      /*
       * Use strstore() below to properly deal with received cookie
       * headers that have the same string property set more than once,
       * and then we use the last one.
       */

      if(!co->name) {
        /* The very first name/value pair is the actual cookie name */
        if(!sep)
          /* Bad name/value pair. */
          return CERR_NO_SEP;

        strstore(&co->name, namep, nlen);
        strstore(&co->value, valuep, vlen);
        done = TRUE;
        if(!co->name || !co->value)
          return CERR_NO_NAME_VALUE;

        if(invalid_octets(co->value) || invalid_octets(co->name)) {
          infof(data, "invalid octets in name/value, cookie dropped");
          return CERR_INVALID_OCTET;
        }
      }
      else if(!vlen) {
        /*
         * this was a "<name>=" with no content, and we must allow
         * 'secure' and 'httponly' specified this weirdly
         */
        done = TRUE;
        /*
         * secure cookies are only allowed to be set when the connection is
         * using a secure protocol, or when the cookie is being set by
         * reading from file
         */
        if((nlen == 6) && strncasecompare("secure", namep, 6)) {
          if(secure || !ci->running) {
            co->secure = TRUE;
          }
          else {
            return CERR_BAD_SECURE;
          }
        }
        else if((nlen == 8) && strncasecompare("httponly", namep, 8))
          co->httponly = TRUE;
        else if(sep)
          /* there was a '=' so we are not done parsing this field */
          done = FALSE;
      }
      if(done)
        ;
      else if((nlen == 4) && strncasecompare("path", namep, 4)) {
        strstore(&co->path, valuep, vlen);
        if(!co->path)
          return CERR_OUT_OF_MEMORY;
        free(co->spath); /* if this is set again */
        co->spath = sanitize_cookie_path(co->path);
        if(!co->spath)
          return CERR_OUT_OF_MEMORY;
      }
      else if((nlen == 6) &&
              strncasecompare("domain", namep, 6) && vlen) {
        bool is_ip;

        /*
         * Now, we make sure that our host is within the given domain, or
         * the given domain is not valid and thus cannot be set.
         */

        if('.' == valuep[0]) {
          valuep++; /* ignore preceding dot */
          vlen--;
        }

#ifndef USE_LIBPSL
        /*
         * Without PSL we do not know when the incoming cookie is set on a
         * TLD or otherwise "protected" suffix. To reduce risk, we require a
         * dot OR the exact hostname being "localhost".
         */
        if(bad_domain(valuep, vlen))
          domain = ":";
#endif

        is_ip = Curl_host_is_ipnum(domain ? domain : valuep);

        if(!domain
           || (is_ip && !strncmp(valuep, domain, vlen) &&
               (vlen == strlen(domain)))
           || (!is_ip && cookie_tailmatch(valuep, vlen, domain))) {
          strstore(&co->domain, valuep, vlen);
          if(!co->domain)
            return CERR_OUT_OF_MEMORY;

          if(!is_ip)
            co->tailmatch = TRUE; /* we always do that if the domain name was
                                     given */
        }
        else {
          /*
           * We did not get a tailmatch and then the attempted set domain is
           * not a domain to which the current host belongs. Mark as bad.
           */
          infof(data, "skipped cookie with bad tailmatch domain: %s",
                valuep);
          return CERR_NO_TAILMATCH;
        }
      }
      else if((nlen == 7) && strncasecompare("version", namep, 7)) {
        /* just ignore */
      }
      else if((nlen == 7) && strncasecompare("max-age", namep, 7)) {
        /*
         * Defined in RFC2109:
         *
         * Optional. The Max-Age attribute defines the lifetime of the
         * cookie, in seconds. The delta-seconds value is a decimal non-
         * negative integer. After delta-seconds seconds elapse, the
         * client should discard the cookie. A value of zero means the
         * cookie should be discarded immediately.
         */
        CURLofft offt;
        const char *maxage = valuep;
        offt = curlx_strtoofft((*maxage == '\"') ?
                               &maxage[1] : &maxage[0], NULL, 10,
                               &co->expires);
        switch(offt) {
        case CURL_OFFT_FLOW:
          /* overflow, used max value */
          co->expires = CURL_OFF_T_MAX;
          break;
   