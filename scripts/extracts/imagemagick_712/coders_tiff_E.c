// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 5/34



    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        break;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      double
        a,
        b;

      a=QuantumScale*(double) GetPixela(image,q)+0.5;
      if (a > 1.0)
        a-=1.0;
      b=QuantumScale*(double) GetPixelb(image,q)+0.5;
      if (b > 1.0)
        b-=1.0;
      SetPixela(image,(Quantum) (QuantumRange*a),q);
      SetPixelb(image,(Quantum) (QuantumRange*b),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      {
        status=MagickFalse;
        break;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

static void ReadProfile(Image *image,const char *name,
  const unsigned char *datum,uint32 length,ExceptionInfo *exception)
{
  StringInfo
    *profile;

  if (length < 4)
    return;
  profile=BlobToProfileStringInfo(name,datum,(size_t) length,
    exception);
  (void) SetImageProfilePrivate(image,profile,exception);
}

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int TIFFCloseBlob(thandle_t image)
{
  (void) CloseBlob((Image *) image);
  return(0);
}

static void TIFFErrors(const char *,const char *,va_list)
  magick_attribute((__format__ (__printf__,2,0)));

static void TIFFErrors(const char *module,const char *format,va_list error)
{
  char
    message[MagickPathExtent];

  ExceptionInfo
    *exception;

#if defined(MAGICKCORE_HAVE_VSNPRINTF)
  (void) vsnprintf(message,MagickPathExtent-2,format,error);
#else
  (void) vsprintf(message,format,error);
#endif
  message[MagickPathExtent-2]='\0';
  (void) ConcatenateMagickString(message,".",MagickPathExtent);
  exception=(ExceptionInfo *) GetMagickThreadValue(tiff_exception);
  if (exception != (ExceptionInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),CoderError,message,
      "`%s'",module);
}

static toff_t TIFFGetBlobSize(thandle_t image)
{
  return((toff_t) GetBlobSize((Image *) image));
}

static void TIFFGetProfiles(TIFF *tiff,Image *image,
  ExceptionInfo *exception)
{
  uint32
    length = 0;

  unsigned char
    *profile = (unsigned char *) NULL;

#if defined(TIFFTAG_ICCPROFILE)
  if ((TIFFGetField(tiff,TIFFTAG_ICCPROFILE,&length,&profile) == 1) &&
      (profile != (unsigned char *) NULL))
    ReadProfile(image,"icc",profile,length,exception);
#endif
#if defined(TIFFTAG_PHOTOSHOP)
  if ((TIFFGetField(tiff,TIFFTAG_PHOTOSHOP,&length,&profile) == 1) &&
      (profile != (unsigned char *) NULL))
    ReadProfile(image,"8bim",profile,length,exception);
#endif
#if defined(TIFFTAG_RICHTIFFIPTC) && (TIFFLIB_VERSION >= 20191103)
  if ((TIFFGetField(tiff,TIFFTAG_RICHTIFFIPTC,&length,&profile) == 1) &&
      (profile != (unsigned char *) NULL))
    {
      const TIFFField
        *field;

      field=TIFFFieldWithTag(tiff,TIFFTAG_RICHTIFFIPTC);
      if ((field != (const TIFFField *) NULL) &&
          (TIFFFieldDataType(field) == TIFF_LONG))
        {
          if (TIFFIsByteSwapped(tiff) != 0)
            TIFFSwabArrayOfLong((uint32 *) profile,(tmsize_t) length);
          ReadProfile(image,"iptc",profile,4L*length,exception);
        }
      else
        ReadProfile(image,"iptc",profile,length,exception);
    }
#endif
#if defined(TIFFTAG_XMLPACKET)
  if ((TIFFGetField(tiff,TIFFTAG_XMLPACKET,&length,&profile) == 1) &&
      (profile != (unsigned char *) NULL))
    {
      StringInfo
        *dng;

      ReadProfile(image,"xmp",profile,length,exception);
      dng=BlobToStringInfo(profile,length);
      if (dng != (StringInfo *) NULL)
        {
          const char
            *target = "dc:format=\"image/dng\"";

          if (strstr((char *) GetStringInfoDatum(dng),target) != (char *) NULL)
            (void) CopyMagickString(image->magick,"DNG",MagickPathExtent);
          dng=DestroyStringInfo(dng);
        }
    }
#endif
  if ((TIFFGetField(tiff,34118,&length,&profile) == 1) &&
      (profile != (unsigned char *) NULL))
    ReadProfile(image,"tiff:34118",profile,length,exception);
  if ((TIFFGetField(tiff,37724,&length,&profile) == 1) &&
      (profile != (unsigned char *) NULL))
    ReadProfile(image,"tiff:37724",profile,length,exception);
}

static MagickBooleanType TIFFGetProperties(TIFF *tiff,Image *image,
  ExceptionInfo *exception)
{
  char
    message[MagickPathExtent],
    *text = (char *) NULL;

  MagickBooleanType
    status;

  uint32
    count = 0,
    type;

  void
    *sans[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };