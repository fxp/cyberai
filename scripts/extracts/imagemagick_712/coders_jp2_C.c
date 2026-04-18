// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jp2.c
// Segment 3/11



  image=(Image *) context;
  count=ReadBlob(image,(size_t) length,(unsigned char *) buffer);
  if (count == 0)
    return((OPJ_SIZE_T) -1);
  return((OPJ_SIZE_T) count);
}

static OPJ_BOOL JP2SeekHandler(OPJ_OFF_T offset,void *context)
{
  Image
    *image;

  image=(Image *) context;
  return(SeekBlob(image,offset,SEEK_SET) < 0 ? OPJ_FALSE : OPJ_TRUE);
}

static OPJ_OFF_T JP2SkipHandler(OPJ_OFF_T offset,void *context)
{
  Image
    *image;

  image=(Image *) context;
  return(SeekBlob(image,offset,SEEK_CUR) < 0 ? -1 : offset);
}

static void JP2WarningHandler(const char *message,void *client_data)
{
  ExceptionInfo
    *exception;

  exception=(ExceptionInfo *) client_data;
  (void) ThrowMagickException(exception,GetMagickModule(),CoderWarning,
    message,"`%s'","OpenJP2");
}

static OPJ_SIZE_T JP2WriteHandler(void *buffer,OPJ_SIZE_T length,void *context)
{
  Image
    *image;

  ssize_t
    count;

  image=(Image *) context;
  count=WriteBlob(image,(size_t) length,(unsigned char *) buffer);
  return((OPJ_SIZE_T) count);
}

static MagickBooleanType JP2ComponentHasAlpha(const ImageInfo* image_info,
  opj_image_comp_t comp)
{
  const char
    *option;

  if (comp.alpha != 0)
    return(MagickTrue);
  option=GetImageOption(image_info,"jp2:assume-alpha");
  return(IsStringTrue(option));
}

static Image *ReadJP2Image(const ImageInfo *image_info,ExceptionInfo *exception)
{
  const char
    *option;

  Image
    *image;

  int
    jp2_status;

  JP2CompsInfo
    comps_info[MaxPixelChannels];

  MagickBooleanType
    status;

  opj_codec_t
    *jp2_codec;

  opj_codestream_info_v2_t
    *jp2_codestream_info;

  opj_dparameters_t
    parameters;

  opj_image_t
    *jp2_image;

  opj_stream_t
    *jp2_stream;

  ssize_t
    i,
    y;

  unsigned char
    magick[16];