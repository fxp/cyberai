/* ===== EXTRACT: http_chunks.c [httpchunk_readwrite] ===== */
/* Lines: 81–420 */
/* File: lib/http_chunks.c — curl 8.11.0 */

void Curl_httpchunk_init(struct Curl_easy *data, struct Curl_chunker *ch,
                         bool ignore_body)
{
  (void)data;
  ch->hexindex = 0;      /* start at 0 */
  ch->state = CHUNK_HEX; /* we get hex first! */
  ch->last_code = CHUNKE_OK;
  Curl_dyn_init(&ch->trailer, DYN_H1_TRAILER);
  ch->ignore_body = ignore_body;
}

void Curl_httpchunk_reset(struct Curl_easy *data, struct Curl_chunker *ch,
                          bool ignore_body)
{
  (void)data;
  ch->hexindex = 0;      /* start at 0 */
  ch->state = CHUNK_HEX; /* we get hex first! */
  ch->last_code = CHUNKE_OK;
  Curl_dyn_reset(&ch->trailer);
  ch->ignore_body = ignore_body;
}

void Curl_httpchunk_free(struct Curl_easy *data, struct Curl_chunker *ch)
{
  (void)data;
  Curl_dyn_free(&ch->trailer);
}

bool Curl_httpchunk_is_done(struct Curl_easy *data, struct Curl_chunker *ch)
{
  (void)data;
  return ch->state == CHUNK_DONE;
}

static CURLcode httpchunk_readwrite(struct Curl_easy *data,
                                    struct Curl_chunker *ch,
                                    struct Curl_cwriter *cw_next,
                                    const char *buf, size_t blen,
                                    size_t *pconsumed)
{
  CURLcode result = CURLE_OK;
  size_t piece;

  *pconsumed = 0; /* nothing's written yet */
  /* first check terminal states that will not progress anywhere */
  if(ch->state == CHUNK_DONE)
    return CURLE_OK;
  if(ch->state == CHUNK_FAILED)
    return CURLE_RECV_ERROR;

  /* the original data is written to the client, but we go on with the
     chunk read process, to properly calculate the content length */
  if(data->set.http_te_skip && !ch->ignore_body) {
    if(cw_next)
      result = Curl_cwriter_write(data, cw_next, CLIENTWRITE_BODY, buf, blen);
    else
      result = Curl_client_write(data, CLIENTWRITE_BODY, (char *)buf, blen);
    if(result) {
      ch->state = CHUNK_FAILED;
      ch->last_code = CHUNKE_PASSTHRU_ERROR;
      return result;
    }
  }

  while(blen) {
    switch(ch->state) {
    case CHUNK_HEX:
      if(ISXDIGIT(*buf)) {
        if(ch->hexindex >= CHUNK_MAXNUM_LEN) {
          failf(data, "chunk hex-length longer than %d", CHUNK_MAXNUM_LEN);
          ch->state = CHUNK_FAILED;
          ch->last_code = CHUNKE_TOO_LONG_HEX; /* longer than we support */
          return CURLE_RECV_ERROR;
        }
        ch->hexbuffer[ch->hexindex++] = *buf;
        buf++;
        blen--;
        (*pconsumed)++;
      }
      else {
        if(0 == ch->hexindex) {
          /* This is illegal data, we received junk where we expected
             a hexadecimal digit. */
          failf(data, "chunk hex-length char not a hex digit: 0x%x", *buf);
          ch->state = CHUNK_FAILED;
          ch->last_code = CHUNKE_ILLEGAL_HEX;
          return CURLE_RECV_ERROR;
        }

        /* blen and buf are unmodified */
        ch->hexbuffer[ch->hexindex] = 0;
        if(curlx_strtoofft(ch->hexbuffer, NULL, 16, &ch->datasize)) {
          failf(data, "chunk hex-length not valid: '%s'", ch->hexbuffer);
          ch->state = CHUNK_FAILED;
          ch->last_code = CHUNKE_ILLEGAL_HEX;
          return CURLE_RECV_ERROR;
        }
        ch->state = CHUNK_LF; /* now wait for the CRLF */
      }
      break;

    case CHUNK_LF:
      /* waiting for the LF after a chunk size */
      if(*buf == 0x0a) {
        /* we are now expecting data to come, unless size was zero! */
        if(0 == ch->datasize) {
          ch->state = CHUNK_TRAILER; /* now check for trailers */
        }
        else {
          ch->state = CHUNK_DATA;
          CURL_TRC_WRITE(data, "http_chunked, chunk start of %"
                         FMT_OFF_T " bytes", ch->datasize);
        }
      }

      buf++;
      blen--;
      (*pconsumed)++;
      break;

    case CHUNK_DATA:
      /* We expect 'datasize' of data. We have 'blen' right now, it can be
         more or less than 'datasize'. Get the smallest piece.
      */
      piece = blen;
      if(ch->datasize < (curl_off_t)blen)
        piece = curlx_sotouz(ch->datasize);

      /* Write the data portion available */
      if(!data->set.http_te_skip && !ch->ignore_body) {
        if(cw_next)
          result = Curl_cwriter_write(data, cw_next, CLIENTWRITE_BODY,
                                      buf, piece);
        else
          result = Curl_client_write(data, CLIENTWRITE_BODY,
                                    (char *)buf, piece);
        if(result) {
          ch->state = CHUNK_FAILED;
          ch->last_code = CHUNKE_PASSTHRU_ERROR;
          return result;
        }
      }

      *pconsumed += piece;
      ch->datasize -= piece; /* decrease amount left to expect */
      buf += piece;    /* move read pointer forward */
      blen -= piece;   /* decrease space left in this round */
      CURL_TRC_WRITE(data, "http_chunked, write %zu body bytes, %"
                     FMT_OFF_T " bytes in chunk remain",
                     piece, ch->datasize);

      if(0 == ch->datasize)
        /* end of data this round, we now expect a trailing CRLF */
        ch->state = CHUNK_POSTLF;
      break;

    case CHUNK_POSTLF:
      if(*buf == 0x0a) {
        /* The last one before we go back to hex state and start all over. */
        Curl_httpchunk_reset(data, ch, ch->ignore_body);
      }
      else if(*buf != 0x0d) {
        ch->state = CHUNK_FAILED;
        ch->last_code = CHUNKE_BAD_CHUNK;
        return CURLE_RECV_ERROR;
      }
      buf++;
      blen--;
      (*pconsumed)++;
      break;

    case CHUNK_TRAILER:
      if((*buf == 0x0d) || (*buf == 0x0a)) {
        char *tr = Curl_dyn_ptr(&ch->trailer);
        /* this is the end of a trailer, but if the trailer was zero bytes
           there was no trailer and we move on */

        if(tr) {
          size_t trlen;
          result = Curl_dyn_addn(&ch->trailer, (char *)STRCONST("\x0d\x0a"));
          if(result) {
            ch->state = CHUNK_FAILED;
            ch->last_code = CHUNKE_OUT_OF_MEMORY;
            return result;
          }
          tr = Curl_dyn_ptr(&ch->trailer);
          trlen = Curl_dyn_len(&ch->trailer);
          if(!data->set.http_te_skip) {
            if(cw_next)
              result = Curl_cwriter_write(data, cw_next,
                                          CLIENTWRITE_HEADER|
                                          CLIENTWRITE_TRAILER,
                                          tr, trlen);
            else
              result = Curl_client_write(data,
                                         CLIENTWRITE_HEADER|
                                         CLIENTWRITE_TRAILER,
                                         tr, trlen);
            if(result) {
              ch->state = CHUNK_FAILED;
              ch->last_code = CHUNKE_PASSTHRU_ERROR;
              return result;
            }
          }
          Curl_dyn_reset(&ch->trailer);
       