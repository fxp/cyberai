// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 3/24



static int GetCharacter(j_decompress_ptr jpeg_info)
{
  if (jpeg_info->src->bytes_in_buffer == 0)
    {
      (void) (*jpeg_info->src->fill_input_buffer)(jpeg_info);
      if (jpeg_info->err->msg_code == JWRN_JPEG_EOF)
        return(EOF);
    }
  jpeg_info->src->bytes_in_buffer--;
  return((int) GETJOCTET(*jpeg_info->src->next_input_byte++));
}

static void InitializeSource(j_decompress_ptr compress_info)
{
  SourceManager
    *source;

  source=(SourceManager *) compress_info->src;
  source->start_of_blob=TRUE;
}

static MagickBooleanType IsAspectRatio(const j_decompress_ptr compress_info)
{
  if (compress_info->density_unit != 0)
    return(MagickFalse);
  if ((compress_info->X_density == 1) && (compress_info->Y_density == 1))
    return(MagickTrue);
  if ((compress_info->X_density == 3) && (compress_info->Y_density == 2))
    return(MagickTrue);
  if ((compress_info->X_density == 4) && (compress_info->Y_density == 3))
    return(MagickTrue);
  if ((compress_info->X_density == 16) && (compress_info->Y_density == 9))
    return(MagickTrue);
  if ((compress_info->X_density == 16) && (compress_info->Y_density == 10))
    return(MagickTrue);
  return(MagickFalse);
}

static MagickBooleanType IsITUFaxImage(const Image *image)
{
  const StringInfo
    *profile;

  const unsigned char
    *datum;

  profile=GetImageProfile(image,"8bim");
  if (profile == (const StringInfo *) NULL)
    return(MagickFalse);
  if (GetStringInfoLength(profile) < 5)
    return(MagickFalse);
  datum=GetStringInfoDatum(profile);
  if ((datum[0] == 0x47) && (datum[1] == 0x33) && (datum[2] == 0x46) &&
      (datum[3] == 0x41) && (datum[4] == 0x58))
    return(MagickTrue);
  return(MagickFalse);
}

static void JPEGErrorHandler(j_common_ptr jpeg_info)
{
  char
    message[JMSG_LENGTH_MAX];

  ExceptionInfo
    *exception;

  Image
    *image;

  JPEGClientInfo
    *client_info;

  *message='\0';
  client_info=(JPEGClientInfo *) jpeg_info->client_data;
  image=client_info->image;
  exception=client_info->exception;
  (jpeg_info->err->format_message)(jpeg_info,message);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "[%s] JPEG Trace: \"%s\"",image->filename,message);
  if (client_info->finished != MagickFalse)
    (void) ThrowMagickException(exception,GetMagickModule(),
      CorruptImageWarning,(char *) message,"`%s'",image->filename);
  else
    (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
      (char *) message,"`%s'",image->filename);
  longjmp(client_info->error_recovery,1);
}

static void JPEGProgressHandler(j_common_ptr jpeg_info)
{
  ExceptionInfo
    *exception;

  Image
    *image;

  JPEGClientInfo
    *client_info;

  client_info=(JPEGClientInfo *) jpeg_info->client_data;
  image=client_info->image;
  exception=client_info->exception;
  if (jpeg_info->is_decompressor == 0)
    return;
  if (((j_decompress_ptr) jpeg_info)->input_scan_number < MaxJPEGScans)
    return;
  (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
    "too many scans","`%s'",image->filename);
  longjmp(client_info->error_recovery,1);
}

static void JPEGWarningHandler(j_common_ptr jpeg_info,int level)
{
#define JPEGExcessiveWarnings  1000

  char
    message[JMSG_LENGTH_MAX];

  ExceptionInfo
    *exception;

  Image
    *image;

  JPEGClientInfo
    *client_info;

  *message='\0';
  client_info=(JPEGClientInfo *) jpeg_info->client_data;
  exception=client_info->exception;
  image=client_info->image;
  if (level < 0)
    {
      /*
        Process warning message.
      */
      (jpeg_info->err->format_message)(jpeg_info,message);
      if (jpeg_info->err->num_warnings++ < JPEGExcessiveWarnings)
        (void) ThrowMagickException(exception,GetMagickModule(),
          CorruptImageWarning,message,"`%s'",image->filename);
    }
  else
    if (level >= jpeg_info->err->trace_level)
      {
        /*
          Process trace message.
        */
        (jpeg_info->err->format_message)(jpeg_info,message);
        if ((image != (Image *) NULL) && (image->debug != MagickFalse))
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "[%s] JPEG Trace: \"%s\"",image->filename,message);
      }
}

static MagickBooleanType ReadProfilePayload(j_decompress_ptr jpeg_info,
  const int index,const size_t length)
{
  ExceptionInfo
    *exception;

  Image
    *image;

  JPEGClientInfo
    *client_info;

  size_t
    offset = 0;

  ssize_t
    i;

  unsigned char
    *p;