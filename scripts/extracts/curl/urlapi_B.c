/* ===== EXTRACT: urlapi.c [parse_hostname_login+ipv6_parse+hostname_check] ===== */
/* Lines: 427–800 */
/* File: lib/urlapi.c — curl 8.11.0 */

static CURLUcode parse_hostname_login(struct Curl_URL *u,
                                      const char *login,
                                      size_t len,
                                      unsigned int flags,
                                      size_t *offset) /* to the hostname */
{
  CURLUcode result = CURLUE_OK;
  CURLcode ccode;
  char *userp = NULL;
  char *passwdp = NULL;
  char *optionsp = NULL;
  const struct Curl_handler *h = NULL;

  /* At this point, we assume all the other special cases have been taken
   * care of, so the host is at most
   *
   *   [user[:password][;options]]@]hostname
   *
   * We need somewhere to put the embedded details, so do that first.
   */
  char *ptr;

  DEBUGASSERT(login);

  *offset = 0;
  ptr = memchr(login, '@', len);
  if(!ptr)
    goto out;

  /* We will now try to extract the
   * possible login information in a string like:
   * ftp://user:password@ftp.my.site:8021/README */
  ptr++;

  /* if this is a known scheme, get some details */
  if(u->scheme)
    h = Curl_get_scheme_handler(u->scheme);

  /* We could use the login information in the URL so extract it. Only parse
     options if the handler says we should. Note that 'h' might be NULL! */
  ccode = Curl_parse_login_details(login, ptr - login - 1,
                                   &userp, &passwdp,
                                   (h && (h->flags & PROTOPT_URLOPTIONS)) ?
                                   &optionsp : NULL);
  if(ccode) {
    result = CURLUE_BAD_LOGIN;
    goto out;
  }

  if(userp) {
    if(flags & CURLU_DISALLOW_USER) {
      /* Option DISALLOW_USER is set and URL contains username. */
      result = CURLUE_USER_NOT_ALLOWED;
      goto out;
    }
    free(u->user);
    u->user = userp;
  }

  if(passwdp) {
    free(u->password);
    u->password = passwdp;
  }

  if(optionsp) {
    free(u->options);
    u->options = optionsp;
  }

  /* the hostname starts at this offset */
  *offset = ptr - login;
  return CURLUE_OK;

out:

  free(userp);
  free(passwdp);
  free(optionsp);
  u->user = NULL;
  u->password = NULL;
  u->options = NULL;

  return result;
}

UNITTEST CURLUcode Curl_parse_port(struct Curl_URL *u, struct dynbuf *host,
                                   bool has_scheme)
{
  char *portptr;
  char *hostname = Curl_dyn_ptr(host);
  /*
   * Find the end of an IPv6 address on the ']' ending bracket.
   */
  if(hostname[0] == '[') {
    portptr = strchr(hostname, ']');
    if(!portptr)
      return CURLUE_BAD_IPV6;
    portptr++;
    /* this is a RFC2732-style specified IP-address */
    if(*portptr) {
      if(*portptr != ':')
        return CURLUE_BAD_PORT_NUMBER;
    }
    else
      portptr = NULL;
  }
  else
    portptr = strchr(hostname, ':');

  if(portptr) {
    char *rest = NULL;
    unsigned long port;
    size_t keep = portptr - hostname;

    /* Browser behavior adaptation. If there is a colon with no digits after,
       just cut off the name there which makes us ignore the colon and just
       use the default port. Firefox, Chrome and Safari all do that.

       Do not do it if the URL has no scheme, to make something that looks like
       a scheme not work!
    */
    Curl_dyn_setlen(host, keep);
    portptr++;
    if(!*portptr)
      return has_scheme ? CURLUE_OK : CURLUE_BAD_PORT_NUMBER;

    if(!ISDIGIT(*portptr))
      return CURLUE_BAD_PORT_NUMBER;

    errno = 0;
    port = strtoul(portptr, &rest, 10);  /* Port number must be decimal */

    if(errno || (port > 0xffff) || *rest)
      return CURLUE_BAD_PORT_NUMBER;

    u->portnum = (unsigned short) port;
    /* generate a new port number string to get rid of leading zeroes etc */
    free(u->port);
    u->port = aprintf("%ld", port);
    if(!u->port)
      return CURLUE_OUT_OF_MEMORY;
  }

  return CURLUE_OK;
}

/* this assumes 'hostname' now starts with [ */
static CURLUcode ipv6_parse(struct Curl_URL *u, char *hostname,
                            size_t hlen) /* length of hostname */
{
  size_t len;
  DEBUGASSERT(*hostname == '[');
  if(hlen < 4) /* '[::]' is the shortest possible valid string */
    return CURLUE_BAD_IPV6;
  hostname++;
  hlen -= 2;

  /* only valid IPv6 letters are ok */
  len = strspn(hostname, "0123456789abcdefABCDEF:.");

  if(hlen != len) {
    hlen = len;
    if(hostname[len] == '%') {
      /* this could now be '%[zone id]' */
      char zoneid[16];
      int i = 0;
      char *h = &hostname[len + 1];
      /* pass '25' if present and is a URL encoded percent sign */
      if(!strncmp(h, "25", 2) && h[2] && (h[2] != ']'))
        h += 2;
      while(*h && (*h != ']') && (i < 15))
        zoneid[i++] = *h++;
      if(!i || (']' != *h))
        return CURLUE_BAD_IPV6;
      zoneid[i] = 0;
      u->zoneid = strdup(zoneid);
      if(!u->zoneid)
        return CURLUE_OUT_OF_MEMORY;
      hostname[len] = ']'; /* insert end bracket */
      hostname[len + 1] = 0; /* terminate the hostname */
    }
    else
      return CURLUE_BAD_IPV6;
    /* hostname is fine */
  }

  /* Normalize the IPv6 address */
  {
    char dest[16]; /* fits a binary IPv6 address */
    hostname[hlen] = 0; /* end the address there */
    if(1 != Curl_inet_pton(AF_INET6, hostname, dest))
      return CURLUE_BAD_IPV6;
    if(Curl_inet_ntop(AF_INET6, dest, hostname, hlen)) {
      hlen = strlen(hostname); /* might be shorter now */
      hostname[hlen + 1] = 0;
    }
    hostname[hlen] = ']'; /* restore ending bracket */
  }
  return CURLUE_OK;
}

static CURLUcode hostname_check(struct Curl_URL *u, char *hostname,
                                size_t hlen) /* length of hostname */
{
  size_t len;
  DEBUGASSERT(hostname);

  if(!hlen)
    return CURLUE_NO_HOST;
  else if(hostname[0] == '[')
    return ipv6_parse(u, hostname, hlen);
  else {
    /* letters from the second string are not ok */
    len = strcspn(hostname, " \r\n\t/:#?!@{}[]\\$\'\"^`*<>=;,+&()%");
    if(hlen != len)
      /* hostname with bad content */
      return CURLUE_BAD_HOSTNAME;
  }
  return CURLUE_OK;
}

/*
 * Handle partial IPv4 numerical addresses and different bases, like
 * '16843009', '0x7f', '0x7f.1' '0177.1.1.1' etc.
 *
 * If the given input string is syntactically wrong IPv4 or any part for
 * example is too big, this function returns HOST_NAME.
 *
 * Output the "normalized" version of that input string in plain quad decimal
 * integers.
 *
 * Returns the host type.
 */

#define HOST_ERROR   -1 /* out of memory */

#define HOST_NAME    1
#define HOST_IPV4    2
#define HOST_IPV6    3

static int ipv4_normalize(struct dynbuf *host)
{
  bool done = FALSE;
  int n = 0;
  const char *c = Curl_dyn_ptr(host);
  unsigned long parts[4] = {0, 0, 0, 0};
  CURLcode result = CURLE_OK;

  if(*c == '[')
    return HOST_IPV6;

  errno = 0; /* for strtoul */
  while(!done) {
    char *endp = NULL;
    unsigned long l;
    if(!ISDIGIT(*c))
      /* most 