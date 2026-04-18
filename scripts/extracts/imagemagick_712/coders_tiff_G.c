// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 7/34



  /*
    Read EXIF properties.
  */
  option=GetImageOption(image_info,"tiff:exif-properties");
  if (IsStringFalse(option) != MagickFalse)
    return;
  offset=0;
  if (TIFFGetField(tiff,TIFFTAG_EXIFIFD,&offset) != 1)
    return;
  directory=TIFFCurrentDirectory(tiff);
  if (TIFFReadEXIFDirectory(tiff,offset) == 1)
    TIFFSetImageProperties(tiff,image,"exif:",exception);
  TIFFSetDirectory(tiff,directory);
}

static void TIFFGetGPSProperties(TIFF *tiff,Image *image,
  const ImageInfo* image_info,ExceptionInfo *exception)
{
#if (TIFFLIB_VERSION >= 20210416)
  const char
    *option;

  tdir_t
    directory;

#if defined(TIFF_VERSION_BIG)
  uint64
#else
  uint32
#endif
    offset;

  /*
    Read GPS properties.
  */
  option=GetImageOption(image_info,"tiff:gps-properties");
  if (IsStringFalse(option) != MagickFalse)
    return;
  offset=0;
  if (TIFFGetField(tiff,TIFFTAG_GPSIFD,&offset) != 1)
    return;
  directory=TIFFCurrentDirectory(tiff);
  if (TIFFReadGPSDirectory(tiff,offset) == 1)
    TIFFSetImageProperties(tiff,image,"exif:GPS",exception);
  TIFFSetDirectory(tiff,directory);
#else
  magick_unreferenced(tiff);
  magick_unreferenced(image);
  magick_unreferenced(image_info);
  magick_unreferenced(exception);
#endif
}

static int TIFFMapBlob(thandle_t image,tdata_t *base,toff_t *size)
{
  *base=(tdata_t *) GetBlobStreamData((Image *) image);
  if (*base != (tdata_t *) NULL)
    *size=(toff_t) GetBlobSize((Image *) image);
  if (*base != (tdata_t *) NULL)
    return(1);
  return(0);
}

static tsize_t TIFFReadBlob(thandle_t image,tdata_t data,tsize_t size)
{
  tsize_t
    count;

  count=(tsize_t) ReadBlob((Image *) image,(size_t) size,(unsigned char *)
    data);
  return(count);
}

static int TIFFReadPixels(TIFF *tiff,const tsample_t sample,
  const ssize_t row,tdata_t scanline)
{
  int
    status;

  status=TIFFReadScanline(tiff,scanline,(uint32) row,sample);
  return(status);
}

static toff_t TIFFSeekBlob(thandle_t image,toff_t offset,int whence)
{
  return((toff_t) SeekBlob((Image *) image,(MagickOffsetType) offset,whence));
}

static void TIFFUnmapBlob(thandle_t image,tdata_t base,toff_t size)
{
  (void) image;
  (void) base;
  (void) size;
}

static void TIFFWarnings(const char *,const char *,va_list)
  magick_attribute((__format__ (__printf__,2,0)));

static void TIFFWarnings(const char *module,const char *format,va_list warning)
{
  char
    message[MagickPathExtent];

  ExceptionInfo
    *exception;

#if defined(MAGICKCORE_HAVE_VSNPRINTF)
  (void) vsnprintf(message,MagickPathExtent-2,format,warning);
#else
  (void) vsprintf(message,format,warning);
#endif
  message[MagickPathExtent-2]='\0';
  (void) ConcatenateMagickString(message,".",MagickPathExtent);
  exception=(ExceptionInfo *) GetMagickThreadValue(tiff_exception);
  if (exception != (ExceptionInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),CoderWarning,
      message,"`%s'",module);
}

static tsize_t TIFFWriteBlob(thandle_t image,tdata_t data,tsize_t size)
{
  tsize_t
    count;

  count=(tsize_t) WriteBlob((Image *) image,(size_t) size,(unsigned char *)
    data);
  return(count);
}

static TIFFMethodType GetJPEGMethod(Image* image,TIFF *tiff,uint16 photometric,
  uint16 bits_per_sample,uint16 samples_per_pixel)
{
#define BUFFER_SIZE 2048

  MagickOffsetType
    position,
    offset;

  size_t
    i;

  TIFFMethodType
    method;

#if defined(TIFF_VERSION_BIG)
  uint64
    *value = (uint64 *) NULL;
#else
  uint32
    *value = (uint32 *) NULL;
#endif

  unsigned char
    buffer[BUFFER_SIZE+32];

  unsigned short
    length;