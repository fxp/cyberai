/* ===== EXTRACT: socks.c [do_SOCKS5] ===== */
/* Lines: 350–700 */
/* File: lib/socks.c — curl 8.11.0 */

#endif
      infof(data, "Hostname '%s' was found", sx->hostname);
      sxstate(sx, data, CONNECT_RESOLVED);
    }
    else {
      result = Curl_resolv_check(data, &dns);
      if(!dns) {
        if(result)
          return CURLPX_RESOLVE_HOST;
        return CURLPX_OK;
      }
    }
    FALLTHROUGH();
  case CONNECT_RESOLVED:
CONNECT_RESOLVED:
  {
    struct Curl_addrinfo *hp = NULL;
    /*
     * We cannot use 'hostent' as a struct that Curl_resolv() returns. It
     * returns a Curl_addrinfo pointer that may not always look the same.
     */
    if(dns) {
      hp = dns->addr;

      /* scan for the first IPv4 address */
      while(hp && (hp->ai_family != AF_INET))
        hp = hp->ai_next;

      if(hp) {
        struct sockaddr_in *saddr_in;
        char buf[64];
        Curl_printable_address(hp, buf, sizeof(buf));

        saddr_in = (struct sockaddr_in *)(void *)hp->ai_addr;
        socksreq[4] = ((unsigned char *)&saddr_in->sin_addr.s_addr)[0];
        socksreq[5] = ((unsigned char *)&saddr_in->sin_addr.s_addr)[1];
        socksreq[6] = ((unsigned char *)&saddr_in->sin_addr.s_addr)[2];
        socksreq[7] = ((unsigned char *)&saddr_in->sin_addr.s_addr)[3];

        infof(data, "SOCKS4 connect to IPv4 %s (locally resolved)", buf);

        Curl_resolv_unlink(data, &dns); /* not used anymore from now on */
      }
      else
        failf(data, "SOCKS4 connection to %s not supported", sx->hostname);
    }
    else
      failf(data, "Failed to resolve \"%s\" for SOCKS4 connect.",
            sx->hostname);

    if(!hp)
      return CURLPX_RESOLVE_HOST;
  }
    FALLTHROUGH();
  case CONNECT_REQ_INIT:
CONNECT_REQ_INIT:
    /*
     * This is currently not supporting "Identification Protocol (RFC1413)".
     */
    socksreq[8] = 0; /* ensure empty userid is NUL-terminated */
    if(sx->proxy_user) {
      size_t plen = strlen(sx->proxy_user);
      if(plen > 255) {
        /* there is no real size limit to this field in the protocol, but
           SOCKS5 limits the proxy user field to 255 bytes and it seems likely
           that a longer field is either a mistake or malicious input */
        failf(data, "Too long SOCKS proxy username");
        return CURLPX_LONG_USER;
      }
      /* copy the proxy name WITH trailing zero */
      memcpy(socksreq + 8, sx->proxy_user, plen + 1);
    }

    /*
     * Make connection
     */
    {
      size_t packetsize = 9 +
        strlen((char *)socksreq + 8); /* size including NUL */

      /* If SOCKS4a, set special invalid IP address 0.0.0.x */
      if(protocol4a) {
        size_t hostnamelen = 0;
        socksreq[4] = 0;
        socksreq[5] = 0;
        socksreq[6] = 0;
        socksreq[7] = 1;
        /* append hostname */
        hostnamelen = strlen(sx->hostname) + 1; /* length including NUL */
        if((hostnamelen <= 255) &&
           (packetsize + hostnamelen < sizeof(sx->buffer)))
          strcpy((char *)socksreq + packetsize, sx->hostname);
        else {
          failf(data, "SOCKS4: too long hostname");
          return CURLPX_LONG_HOSTNAME;
        }
        packetsize += hostnamelen;
      }
      sx->outp = socksreq;
      DEBUGASSERT(packetsize <= sizeof(sx->buffer));
      sx->outstanding = packetsize;
      sxstate(sx, data, CONNECT_REQ_SENDING);
    }
    FALLTHROUGH();
  case CONNECT_REQ_SENDING:
    /* Send request */
    presult = socks_state_send(cf, sx, data, CURLPX_SEND_CONNECT,
                               "SOCKS4 connect request");
    if(CURLPX_OK != presult)
      return presult;
    else if(sx->outstanding) {
      /* remain in sending state */
      return CURLPX_OK;
    }
    /* done sending! */
    sx->outstanding = 8; /* receive data size */
    sx->outp = socksreq;
    sxstate(sx, data, CONNECT_SOCKS_READ);

    FALLTHROUGH();
  case CONNECT_SOCKS_READ:
    /* Receive response */
    presult = socks_state_recv(cf, sx, data, CURLPX_RECV_CONNECT,
                               "connect request ack");
    if(CURLPX_OK != presult)
      return presult;
    else if(sx->outstanding) {
      /* remain in reading state */
      return CURLPX_OK;
    }
    sxstate(sx, data, CONNECT_DONE);
    break;
  default: /* lots of unused states in SOCKS4 */
    break;
  }

  /*
   * Response format
   *
   *     +----+----+----+----+----+----+----+----+
   *     | VN | CD | DSTPORT |      DSTIP        |
   *     +----+----+----+----+----+----+----+----+
   * # of bytes:  1    1      2              4
   *
   * VN is the version of the reply code and should be 0. CD is the result
   * code with one of the following values:
   *
   * 90: request granted
   * 91: request rejected or failed
   * 92: request rejected because SOCKS server cannot connect to
   *     identd on the client
   * 93: request rejected because the client program and identd
   *     report different user-ids
   */

  /* wrong version ? */
  if(socksreq[0]) {
    failf(data,
          "SOCKS4 reply has wrong version, version should be 0.");
    return CURLPX_BAD_VERSION;
  }

  /* Result */
  switch(socksreq[1]) {
  case 90:
    infof(data, "SOCKS4%s request granted.", protocol4a ? "a" : "");
    break;
  case 91:
    failf(data,
          "cannot complete SOCKS4 connection to %d.%d.%d.%d:%d. (%d)"
          ", request rejected or failed.",
          socksreq[4], socksreq[5], socksreq[6], socksreq[7],
          (((unsigned char)socksreq[2] << 8) | (unsigned char)socksreq[3]),
          (unsigned char)socksreq[1]);
    return CURLPX_REQUEST_FAILED;
  case 92:
    failf(data,
          "cannot complete SOCKS4 connection to %d.%d.%d.%d:%d. (%d)"
          ", request rejected because SOCKS server cannot connect to "
          "identd on the client.",
          socksreq[4], socksreq[5], socksreq[6], socksreq[7],
          (((unsigned char)socksreq[2] << 8) | (unsigned char)socksreq[3]),
          (unsigned char)socksreq[1]);
    return CURLPX_IDENTD;
  case 93:
    failf(data,
          "cannot complete SOCKS4 connection to %d.%d.%d.%d:%d. (%d)"
          ", request rejected because the client program and identd "
          "report different user-ids.",
          socksreq[4], socksreq[5], socksreq[6], socksreq[7],
          (((unsigned char)socksreq[2] << 8) | (unsigned char)socksreq[3]),
          (unsigned char)socksreq[1]);
    return CURLPX_IDENTD_DIFFER;
  default:
    failf(data,
          "cannot complete SOCKS4 connection to %d.%d.%d.%d:%d. (%d)"
          ", Unknown.",
          socksreq[4], socksreq[5], socksreq[6], socksreq[7],
          (((unsigned char)socksreq[2] << 8) | (unsigned char)socksreq[3]),
          (unsigned char)socksreq[1]);
    return CURLPX_UNKNOWN_FAIL;
  }

  return CURLPX_OK; /* Proxy was successful! */
}

/*
 * This function logs in to a SOCKS5 proxy and sends the specifics to the final
 * destination server.
 */
static CURLproxycode do_SOCKS5(struct Curl_cfilter *cf,
                               