/* ===== EXTRACT: ftp.c [match_pasv_6nums+ftp_state_pasv_resp] ===== */
/* Lines: 1754–1960 */
/* File: lib/ftp.c — curl 8.11.0 */

static bool match_pasv_6nums(const char *p,
                             unsigned int *array) /* 6 numbers */
{
  int i;
  for(i = 0; i < 6; i++) {
    unsigned long num;
    char *endp;
    if(i) {
      if(*p != ',')
        return FALSE;
      p++;
    }
    if(!ISDIGIT(*p))
      return FALSE;
    num = strtoul(p, &endp, 10);
    if(num > 255)
      return FALSE;
    array[i] = (unsigned int)num;
    p = endp;
  }
  return TRUE;
}

static CURLcode ftp_state_pasv_resp(struct Curl_easy *data,
                                    int ftpcode)
{
  struct connectdata *conn = data->conn;
  struct ftp_conn *ftpc = &conn->proto.ftpc;
  CURLcode result;
  struct Curl_dns_entry *addr = NULL;
  enum resolve_t rc;
  unsigned short connectport; /* the local port connect() should use! */
  struct pingpong *pp = &ftpc->pp;
  char *str =
    Curl_dyn_ptr(&pp->recvbuf) + 4; /* start on the first letter */

  /* if we come here again, make sure the former name is cleared */
  Curl_safefree(ftpc->newhost);

  if((ftpc->count1 == 0) &&
     (ftpcode == 229)) {
    /* positive EPSV response */
    char *ptr = strchr(str, '(');
    if(ptr) {
      char sep;
      ptr++;
      /* |||12345| */
      sep = ptr[0];
      /* the ISDIGIT() check here is because strtoul() accepts leading minus
         etc */
      if((ptr[1] == sep) && (ptr[2] == sep) && ISDIGIT(ptr[3])) {
        char *endp;
        unsigned long num = strtoul(&ptr[3], &endp, 10);
        if(*endp != sep)
          ptr = NULL;
        else if(num > 0xffff) {
          failf(data, "Illegal port number in EPSV reply");
          return CURLE_FTP_WEIRD_PASV_REPLY;
        }
        if(ptr) {
          ftpc->newport = (unsigned short)(num & 0xffff);
          ftpc->newhost = strdup(control_address(conn));
          if(!ftpc->newhost)
            return CURLE_OUT_OF_MEMORY;
        }
      }
      else
        ptr = NULL;
    }
    if(!ptr) {
      failf(data, "Weirdly formatted EPSV reply");
      return CURLE_FTP_WEIRD_PASV_REPLY;
    }
  }
  else if((ftpc->count1 == 1) &&
          (ftpcode == 227)) {
    /* positive PASV response */
    unsigned int ip[6];

    /*
     * Scan for a sequence of six comma-separated numbers and use them as
     * IP+port indicators.
     *
     * Found reply-strings include:
     * "227 Entering Passive Mode (127,0,0,1,4,51)"
     * "227 Data transfer will passively listen to 127,0,0,1,4,51"
     * "227 Entering passive mode. 127,0,0,1,4,51"
     */
    while(*str) {
      if(match_pasv_6nums(str, ip))
        break;
      str++;
    }

    if(!*str) {
      failf(data, "Couldn't interpret the 227-response");
      return CURLE_FTP_WEIRD_227_FORMAT;
    }

    /* we got OK from server */
    if(data->set.ftp_skip_ip) {
      /* told to ignore the remotely given IP but instead use the host we used
         for the control connection */
      infof(data, "Skip %u.%u.%u.%u for data connection, reuse %s instead",
            ip[0], ip[1], ip[2], ip[3],
            conn->host.name);
      ftpc->newhost = strdup(control_address(conn));
    }
    else
      ftpc->newhost = aprintf("%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);

    if(!ftpc->newhost)
      return CURLE_OUT_OF_MEMORY;

    ftpc->newport = (unsigned short)(((ip[4] << 8) + ip[5]) & 0xffff);
  }
  else if(ftpc->count1 == 0) {
    /* EPSV failed, move on to PASV */
    return ftp_epsv_disable(data, conn);
  }
  else {
    failf(data, "Bad PASV/EPSV response: %03d", ftpcode);
    return CURLE_FTP_WEIRD_PASV_REPLY;
  }

#ifndef CURL_DISABLE_PROXY
  if(conn->bits.proxy) {
    /*
     * This connection uses a proxy and we need to connect to the proxy again
     * here. We do not want to rely on a former host lookup that might've
     * expired now, instead we remake the lookup here and now!
     */
    const char * const host_name = conn->bits.socksproxy ?
      conn->socks_proxy.host.name : conn->http_proxy.host.name;
    rc = Curl_resolv(data, host_name, conn->primary.remote_port, FALSE, &addr);
    if(rc == CURLRESOLV_PENDING)
      /* BLOCKING, ignores the return code but 'addr' will be NULL in
         case of failure */
      (void)Curl_resolver_wait_resolv(data, &addr);

    /* we connect to the proxy's port */
    connectport = (unsigned short)conn->primary.remote_port;

    if(!addr) {
      failf(data, "cannot resolve proxy host %s:%hu", host_name, connectport);
      return CURLE_COULDNT_RESOLVE_PROXY;
    }
  }
  else
#endif
  {
    /* normal, direct, ftp connection */
    DEBUGASSERT(ftpc->newhost);

    /* postponed address resolution in case of tcp fastopen */
    if(conn->bits.tcp_fastopen && !conn->bits.reuse && !ftpc->newhost[0]) {
      Curl_safefree(ftpc->newhost);
      ftpc->newhost = strdup(control_address(conn));
      if(!ftpc->newhost)
        return CURLE_OUT_OF_MEMORY;
    }

    rc = Curl_resolv(data, ftpc->newhost, ftpc->newport, FALSE, &addr);
    if(rc == CURLRESOLV_PENDING)
      /* BLOCKING */
      (void)Curl_resolver_wait_resolv(data, &addr);

    connectport = ftpc->newport; /* we connect to the remote port */

    if(!addr) {
      failf(data, "cannot resolve new host %s:%hu",
            ftpc->newhost, connectport);
      return CURLE_FTP_CANT_GET_HOST;
    }
  }

  result = Curl_conn_setup(data, conn, SECONDARYSOCKET, addr,
                           conn->bits.ftp_use_data_ssl ?
                           CURL_CF_SSL_ENABLE : CURL_CF_SSL_DISABLE);

  if(result) {
    Curl_resolv_unlink(data, &addr); /* we are done using this address */
    if(ftpc->count1 == 0 && ftpcode == 229)
      return ftp_epsv_disable(data, conn);

    return result;
  }


  /*
   * When this is used from the multi interface, this might've returned with
   * the 'connected' set to FALSE and thus we are now awaiting a non-blocking
   * connect to connect.
   */

  if(data->set.verbose)
    /* this just dumps information about this second connection */
    ftp_pasv_verbose(data, addr->addr, ftpc->newhost, connectport);

  Curl_resolv_unlink(data, &addr); /* we are done using this address */

  Curl_safefree(conn->secondaryhostname);
  conn->secondary_port = ftpc->newport;
  conn->secondaryhostname = strdup(ftpc->newhost);
  if(!conn->secondaryhostname)
    return CURLE_OUT_OF_MEMORY;

