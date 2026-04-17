/* ===== EXTRACT: ftp.c [ftp_state_use_port+use_pasv] ===== */
/* Lines: 1680–1780 */
/* File: lib/ftp.c — curl 8.11.0 */

               In addition: asking for the size for 'TYPE A' transfers is not
               constructive since servers do not report the converted size. So
               skip it.
            */
            result = Curl_pp_sendf(data, &ftpc->pp, "RETR %s", ftpc->file);
            if(!result)
              ftp_state(data, FTP_RETR);
          }
          else {
            result = Curl_pp_sendf(data, &ftpc->pp, "SIZE %s", ftpc->file);
            if(!result)
              ftp_state(data, FTP_RETR_SIZE);
          }
        }
      }
      break;
    case FTP_STOR_PREQUOTE:
      result = ftp_state_ul_setup(data, FALSE);
      break;
    case FTP_POSTQUOTE:
      break;
    }
  }

  return result;
}

/* called from ftp_state_pasv_resp to switch to PASV in case of EPSV
   problems */
static CURLcode ftp_epsv_disable(struct Curl_easy *data,
                                 struct connectdata *conn)
{
  CURLcode result = CURLE_OK;

  if(conn->bits.ipv6
#ifndef CURL_DISABLE_PROXY
     && !(conn->bits.tunnel_proxy || conn->bits.socksproxy)
#endif
    ) {
    /* We cannot disable EPSV when doing IPv6, so this is instead a fail */
    failf(data, "Failed EPSV attempt, exiting");
    return CURLE_WEIRD_SERVER_REPLY;
  }

  infof(data, "Failed EPSV attempt. Disabling EPSV");
  /* disable it for next transfer */
  conn->bits.ftp_use_epsv = FALSE;
  Curl_conn_close(data, SECONDARYSOCKET);
  Curl_conn_cf_discard_all(data, conn, SECONDARYSOCKET);
  data->state.errorbuf = FALSE; /* allow error message to get
                                         rewritten */
  result = Curl_pp_sendf(data, &conn->proto.ftpc.pp, "%s", "PASV");
  if(!result) {
    conn->proto.ftpc.count1++;
    /* remain in/go to the FTP_PASV state */
    ftp_state(data, FTP_PASV);
  }
  return result;
}


static char *control_address(struct connectdata *conn)
{
  /* Returns the control connection IP address.
     If a proxy tunnel is used, returns the original hostname instead, because
     the effective control connection address is the proxy address,
     not the ftp host. */
#ifndef CURL_DISABLE_PROXY
  if(conn->bits.tunnel_proxy || conn->bits.socksproxy)
    return conn->host.name;
#endif
  return conn->primary.remote_ip;
}

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
