/* ===== EXTRACT: ftplistparser.c [state machine] ===== */
/* Lines: 100–600 */
/* File: lib/ftplistparser.c — curl 8.11.0 */

    PL_UNIX_TIME_PREPART2,
    PL_UNIX_TIME_PART2,
    PL_UNIX_TIME_PREPART3,
    PL_UNIX_TIME_PART3
  } time;

  enum {
    PL_UNIX_FILENAME_PRESPACE = 0,
    PL_UNIX_FILENAME_NAME,
    PL_UNIX_FILENAME_WINDOWSEOL
  } filename;

  enum {
    PL_UNIX_SYMLINK_PRESPACE = 0,
    PL_UNIX_SYMLINK_NAME,
    PL_UNIX_SYMLINK_PRETARGET1,
    PL_UNIX_SYMLINK_PRETARGET2,
    PL_UNIX_SYMLINK_PRETARGET3,
    PL_UNIX_SYMLINK_PRETARGET4,
    PL_UNIX_SYMLINK_TARGET,
    PL_UNIX_SYMLINK_WINDOWSEOL
  } symlink;
} pl_unix_substate;

typedef enum {
  PL_WINNT_DATE = 0,
  PL_WINNT_TIME,
  PL_WINNT_DIRORSIZE,
  PL_WINNT_FILENAME
} pl_winNT_mainstate;

typedef union {
  enum {
    PL_WINNT_TIME_PRESPACE = 0,
    PL_WINNT_TIME_TIME
  } time;
  enum {
    PL_WINNT_DIRORSIZE_PRESPACE = 0,
    PL_WINNT_DIRORSIZE_CONTENT
  } dirorsize;
  enum {
    PL_WINNT_FILENAME_PRESPACE = 0,
    PL_WINNT_FILENAME_CONTENT,
    PL_WINNT_FILENAME_WINEOL
  } filename;
} pl_winNT_substate;

/* This struct is used in wildcard downloading - for parsing LIST response */
struct ftp_parselist_data {
  enum {
    OS_TYPE_UNKNOWN = 0,
    OS_TYPE_UNIX,
    OS_TYPE_WIN_NT
  } os_type;

  union {
    struct {
      pl_unix_mainstate main;
      pl_unix_substate sub;
    } UNIX;

    struct {
      pl_winNT_mainstate main;
      pl_winNT_substate sub;
    } NT;
  } state;

  CURLcode error;
  struct fileinfo *file_data;
  unsigned int item_length;
  size_t item_offset;
  struct {
    size_t filename;
    size_t user;
    size_t group;
    size_t time;
    size_t perm;
    size_t symlink_target;
  } offsets;
};

static void fileinfo_dtor(void *user, void *element)
{
  (void)user;
  Curl_fileinfo_cleanup(element);
}

CURLcode Curl_wildcard_init(struct WildcardData *wc)
{
  Curl_llist_init(&wc->filelist, fileinfo_dtor);
  wc->state = CURLWC_INIT;

  return CURLE_OK;
}

void Curl_wildcard_dtor(struct WildcardData **wcp)
{
  struct WildcardData *wc = *wcp;
  if(!wc)
    return;

  if(wc->dtor) {
    wc->dtor(wc->ftpwc);
    wc->dtor = ZERO_NULL;
    wc->ftpwc = NULL;
  }
  DEBUGASSERT(wc->ftpwc == NULL);

  Curl_llist_destroy(&wc->filelist, NULL);
  free(wc->path);
  wc->path = NULL;
  free(wc->pattern);
  wc->pattern = NULL;
  wc->state = CURLWC_INIT;
  free(wc);
  *wcp = NULL;
}

struct ftp_parselist_data *Curl_ftp_parselist_data_alloc(void)
{
  return calloc(1, sizeof(struct ftp_parselist_data));
}


void Curl_ftp_parselist_data_free(struct ftp_parselist_data **parserp)
{
  struct ftp_parselist_data *parser = *parserp;
  if(parser)
    Curl_fileinfo_cleanup(parser->file_data);
  free(parser);
  *parserp = NULL;
}


CURLcode Curl_ftp_parselist_geterror(struct ftp_parselist_data *pl_data)
{
  return pl_data->error;
}


#define FTP_LP_MALFORMATED_PERM 0x01000000

static unsigned int ftp_pl_get_permission(const char *str)
{
  unsigned int permissions = 0;
  /* USER */
  if(str[0] == 'r')
    permissions |= 1 << 8;
  else if(str[0] != '-')
    permissions |= FTP_LP_MALFORMATED_PERM;
  if(str[1] == 'w')
    permissions |= 1 << 7;
  else if(str[1] != '-')
    permissions |= FTP_LP_MALFORMATED_PERM;

  if(str[2] == 'x')
    permissions |= 1 << 6;
  else if(str[2] == 's') {
    permissions |= 1 << 6;
    permissions |= 1 << 11;
  }
  else if(str[2] == 'S')
    permissions |= 1 << 11;
  else if(str[2] != '-')
    permissions |= FTP_LP_MALFORMATED_PERM;
  /* GROUP */
  if(str[3] == 'r')
    permissions |= 1 << 5;
  else if(str[3] != '-')
    permissions |= FTP_LP_MALFORMATED_PERM;
  if(str[4] == 'w')
    permissions |= 1 << 4;
  else if(str[4] != '-')
    permissions |= FTP_LP_MALFORMATED_PERM;
  if(str[5] == 'x')
    permissions |= 1 << 3;
  else if(str[5] == 's') {
    permissions |= 1 << 3;
    permissions |= 1 << 10;
  }
  else if(str[5] == 'S')
    permissions |= 1 << 10;
  else if(str[5] != '-')
    permissions |= FTP_LP_MALFORMATED_PERM;
  /* others */
  if(str[6] == 'r')
    permissions |= 1 << 2;
  else if(str[6] != '-')
    permissions |= FTP_LP_MALFORMATED_PERM;
  if(str[7] == 'w')
    permissions |= 1 << 1;
  else if(str[7] != '-')
      permissions |= FTP_LP_MALFORMATED_PERM;
  if(str[8] == 'x')
    permissions |= 1;
  else if(str[8] == 't') {
    permissions |= 1;
    permissions |= 1 << 9;
  }
  else if(str[8] == 'T')
    permissions |= 1 << 9;
  else if(str[8] != '-')
    permissions |= FTP_LP_MALFORMATED_PERM;

  return permissions;
}

static CURLcode ftp_pl_insert_finfo(struct Curl_easy *data,
                                    struct fileinfo *infop)
{
  curl_fnmatch_callback compare;
  struct WildcardData *wc = data->wildcard;
  struct ftp_wc *ftpwc = wc->ftpwc;
  struct Curl_llist *llist = &wc->filelist;
  struct ftp_parselist_data *parser = ftpwc->parser;
  bool add = TRUE;
  struct curl_fileinfo *finfo = &infop->info;

  /* set the finfo pointers */
  char *str = Curl_dyn_ptr(&infop->buf);
  finfo->filename       = str + parser->offsets.filename;
  finfo->strings.group  = parser->offsets.group ?
                          str + parser->offsets.group : NULL;
  finfo->strings.perm   = parser->offsets.perm ?
                          str + parser->offsets.perm : NULL;
  finfo->strings.target = parser->offsets.symlink_target ?
                          str + parser->offsets.symlink_target : NULL;
  finfo->strings.time   = str + parser->offsets.time;
  finfo->strings.user   = parser->offsets.user ?
                          str + parser->offsets.user : NULL;

  /* get correct fnmatch callback */
  compare = data->set.fnmatch;
  if(!compare)
    compare = Curl_fnmatch;

  /* filter pattern-corresponding filenames */
  Curl_set_in_callback(data, TRUE);
  if(compare(data->set.fnmatch_data, wc->pattern,
             finfo->filename) == 0) {
    /* discard symlink which is containing multiple " -> " */
    if((finfo->filetype == CURLFILETYPE_SYMLINK) && finfo->strings.target &&
       (strstr(finfo->strings.target, " -> "))) {
      add = FALSE;
    }
  }
  else {
    add = FALSE;
  }
  Curl_set_in_callback(data, FALSE);

  if(add) {
    Curl_llist_append(llist, finfo, &infop->list);
  }
  else {
    Curl_fileinfo_cleanup(infop);
  }

  ftpwc->parser->file_data = NULL;
  return CURLE_OK;
}

#define MAX_FTPLIST_BUFFER 10000 /* arbitrarily set */

size_t Curl_ftp_parselist(char *buffer, size_t size, size_t nmemb,
                          void *connptr)
{
  size_t bufflen = size*nmemb;
  struct Curl_easy *data = (struct Curl_easy *)connptr;
  struct ftp_wc *ftpwc = data->wildcard->ftpwc;
  struct ftp_parselist_data *parser = ftpwc->parser;
  size_t i = 0;
  CURLcode result;
  size_t retsize = bufflen;

  if(parser->error) { /* error in previous call */
    /* scenario:
     * 1. call => OK..
     * 2. call => OUT_OF_MEMORY (or other error)
     * 3. (last) call => is skipped RIGHT HERE and the error is handled later
     *    in wc_statemach()
     */
    goto fail;
  }

  if(parser->os_type == OS_T