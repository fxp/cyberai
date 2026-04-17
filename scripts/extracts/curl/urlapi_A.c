/* ===== EXTRACT: urlapi.c [parseurl] ===== */
/* Lines: 992–1350 */
/* File: lib/urlapi.c — curl 8.11.0 */

static CURLUcode parseurl(const char *url, CURLU *u, unsigned int flags)
{
  const char *path;
  size_t pathlen;
  char *query = NULL;
  char *fragment = NULL;
  char schemebuf[MAX_SCHEME_LEN + 1];
  size_t schemelen = 0;
  size_t urllen;
  CURLUcode result = CURLUE_OK;
  size_t fraglen = 0;
  struct dynbuf host;

  DEBUGASSERT(url);

  Curl_dyn_init(&host, CURL_MAX_INPUT_LENGTH);

  result = junkscan(url, &urllen, flags);
  if(result)
    goto fail;

  schemelen = Curl_is_absolute_url(url, schemebuf, sizeof(schemebuf),
                                   flags & (CURLU_GUESS_SCHEME|
                                            CURLU_DEFAULT_SCHEME));

  /* handle the file: scheme */
  if(schemelen && !strcmp(schemebuf, "file")) {
    bool uncpath = FALSE;
    if(urllen <= 6) {
      /* file:/ is not enough to actually be a complete file: URL */
      result = CURLUE_BAD_FILE_URL;
      goto fail;
    }

    /* path has been allocated large enough to hold this */
    path = (char *)&url[5];
    pathlen = urllen - 5;

    u->scheme = strdup("file");
    if(!u->scheme) {
      result = CURLUE_OUT_OF_MEMORY;
      goto fail;
    }

    /* Extra handling URLs with an authority component (i.e. that start with
     * "file://")
     *
     * We allow omitted hostname (e.g. file:/<path>) -- valid according to
     * RFC 8089, but not the (current) WHAT-WG URL spec.
     */
    if(path[0] == '/' && path[1] == '/') {
      /* swallow the two slashes */
      const char *ptr = &path[2];

      /*
       * According to RFC 8089, a file: URL can be reliably dereferenced if:
       *
       *  o it has no/blank hostname, or
       *
       *  o the hostname matches "localhost" (case-insensitively), or
       *
       *  o the hostname is a FQDN that resolves to this machine, or
       *
       *  o it is an UNC String transformed to an URI (Windows only, RFC 8089
       *    Appendix E.3).
       *
       * For brevity, we only consider URLs with empty, "localhost", or
       * "127.0.0.1" hostnames as local, otherwise as an UNC String.
       *
       * Additionally, there is an exception for URLs with a Windows drive
       * letter in the authority (which was accidentally omitted from RFC 8089
       * Appendix E, but believe me, it was meant to be there. --MK)
       */
      if(ptr[0] != '/' && !STARTS_WITH_URL_DRIVE_PREFIX(ptr)) {
        /* the URL includes a hostname, it must match "localhost" or
           "127.0.0.1" to be valid */
        if(checkprefix("localhost/", ptr) ||
           checkprefix("127.0.0.1/", ptr)) {
          ptr += 9; /* now points to the slash after the host */
        }
        else {
#if defined(_WIN32)
          size_t len;

          /* the hostname, NetBIOS computer name, can not contain disallowed
             chars, and the delimiting slash character must be appended to the
             hostname */
          path = strpbrk(ptr, "/\\:*?\"<>|");
          if(!path || *path != '/') {
            result = CURLUE_BAD_FILE_URL;
            goto fail;
          }

          len = path - ptr;
          if(len) {
            CURLcode code = Curl_dyn_addn(&host, ptr, len);
            if(code) {
              result = cc2cu(code);
              goto fail;
            }
            uncpath = TRUE;
          }

          ptr -= 2; /* now points to the // before the host in UNC */
#else
          /* Invalid file://hostname/, expected localhost or 127.0.0.1 or
             none */
          result = CURLUE_BAD_FILE_URL;
          goto fail;
#endif
        }
      }

      path = ptr;
      pathlen = urllen - (ptr - url);
    }

    if(!uncpath)
      /* no host for file: URLs by default */
      Curl_dyn_reset(&host);

#if !defined(_WIN32) && !defined(MSDOS) && !defined(__CYGWIN__)
    /* Do not allow Windows drive letters when not in Windows.
     * This catches both "file:/c:" and "file:c:" */
    if(('/' == path[0] && STARTS_WITH_URL_DRIVE_PREFIX(&path[1])) ||
       STARTS_WITH_URL_DRIVE_PREFIX(path)) {
      /* File drive letters are only accepted in MS-DOS/Windows */
      result = CURLUE_BAD_FILE_URL;
      goto fail;
    }
#else
    /* If the path starts with a slash and a drive letter, ditch the slash */
    if('/' == path[0] && STARTS_WITH_URL_DRIVE_PREFIX(&path[1])) {
      /* This cannot be done with strcpy, as the memory chunks overlap! */
      path++;
      pathlen--;
    }
#endif

  }
  else {
    /* clear path */
    const char *schemep = NULL;
    const char *hostp;
    size_t hostlen;

    if(schemelen) {
      int i = 0;
      const char *p = &url[schemelen + 1];
      while((*p == '/') && (i < 4)) {
        p++;
        i++;
      }

      schemep = schemebuf;
      if(!Curl_get_scheme_handler(schemep) &&
         !(flags & CURLU_NON_SUPPORT_SCHEME)) {
        result = CURLUE_UNSUPPORTED_SCHEME;
        goto fail;
      }

      if((i < 1) || (i > 3)) {
        /* less than one or more than three slashes */
        result = CURLUE_BAD_SLASHES;
        goto fail;
      }
      hostp = p; /* hostname starts here */
    }
    else {
      /* no scheme! */

      if(!(flags & (CURLU_DEFAULT_SCHEME|CURLU_GUESS_SCHEME))) {
        result = CURLUE_BAD_SCHEME;
        goto fail;
      }
      if(flags & CURLU_DEFAULT_SCHEME)
        schemep = DEFAULT_SCHEME;

      /*
       * The URL was badly formatted, let's try without scheme specified.
       */
      hostp = url;
    }

    if(schemep) {
      u->scheme = strdup(schemep);
      if(!u->scheme) {
        result = CURLUE_OUT_OF_MEMORY;
        goto fail;
      }
    }

    /* find the end of the hostname + port number */
    hostlen = strcspn(hostp, "/?#");
    path = &hostp[hostlen];

    /* this pathlen also contains the query and the fragment */
    pathlen = urllen - (path - url);
    if(hostlen) {

      result = parse_authority(u, hostp, hostlen, flags, &host, schemelen);
      if(result)
        goto fail;

      if((flags & CURLU_GUESS_SCHEME) && !schemep) {
        const char *hostname = Curl_dyn_ptr(&host);
        /* legacy curl-style guess based on hostname */
        if(checkprefix("ftp.", hostname))
          schemep = "ftp";
        else if(checkprefix("dict.", hostname))
          schemep = "dict";
        else if(checkprefix("ldap.", hostname))
          schemep = "ldap";
        else if(checkprefix("imap.", hostname))
          schemep = "imap";
        else if(checkprefix("smtp.", hostname))
          schemep = "smtp";
        else if(checkprefix("pop3.", hostname))
          schemep = "pop3";
        else
          schemep = "http";

        u->scheme = strdup(schemep);
        if(!u->scheme) {
          result = CURLUE_OUT_OF_MEMORY;
          goto fail;
        }
        u->guessed_scheme = TRUE;
      }
    }
    else if(flags & CURLU_NO_AUTHORITY) {
      /* allowed to be empty. */
      if(Curl_dyn_add(&host, "")) {
        result = CURLUE_OUT_OF_MEMORY;
     