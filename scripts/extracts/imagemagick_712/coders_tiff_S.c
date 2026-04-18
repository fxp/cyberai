// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 19/34



    p=q;
    count++;

    while ((isspace((int) ((unsigned char) *p)) != 0) || (*p == ','))
      p++;
  }
  if (count == 0)
    return;
  i=0;
  p=tags;
  ignore=(TIFFFieldInfo *) AcquireQuantumMemory(count,sizeof(*ignore));
  if (ignore == (TIFFFieldInfo *) NULL)
    return;
  /*
    This also sets field_bit to 0 (FIELD_IGNORE).
  */
  (void) memset(ignore,0,count*sizeof(*ignore));
  while (*p != '\0')
  {
    while ((isspace((int) ((unsigned char) *p)) != 0))
      p++;

    ignore[i].field_tag=(ttag_t) strtol(p,&q,10);
    ignore[i].field_name=(char *) dummy_name;

    p=q;
    i++;

    while ((isspace((int) ((unsigned char) *p)) != 0) || (*p == ','))
      p++;
  }
  (void) TIFFMergeFieldInfo(tiff,ignore,(uint32) count);
  ignore=(TIFFFieldInfo *) RelinquishMagickMemory(ignore);
}

static void TIFFTagExtender(TIFF *tiff)
{
  static const TIFFFieldInfo
    TIFFExtensions[] =
    {
      { 37724, -3, -3, TIFF_UNDEFINED, FIELD_CUSTOM, 1, 1,
        (char *) "PhotoshopLayerData" },
      { 34118, -3, -3, TIFF_UNDEFINED, FIELD_CUSTOM, 1, 1,
        (char *) "Microscope" }
    };

  TIFFMergeFieldInfo(tiff,TIFFExtensions,sizeof(TIFFExtensions)/
    sizeof(*TIFFExtensions));
  if (tag_extender != (TIFFExtendProc) NULL)
    (*tag_extender)(tiff);
  TIFFIgnoreTags(tiff);
}
#endif

ModuleExport size_t RegisterTIFFImage(void)
{
#define TIFFDescription  "Tagged Image File Format"

  char
    version[MagickPathExtent];

  MagickInfo
    *entry;

  static const char
    TIFFNote[] =
      "Compression options: "
#if defined(COMPRESSION_NONE)
      "None"
#endif
#if defined(COMPRESSION_CCITTFAX3)
      ", Fax/Group3"
#endif
#if defined(COMPRESSION_CCITTFAX4)
      ", Group4"
#endif
#if defined(COMPRESSION_JBIG)
      ", JBIG"
#endif
#if defined(COMPRESSION_JPEG)
      ", JPEG"
#endif
#if defined(COMPRESSION_LERC)
      ", LERC"
#endif
#if defined(COMPRESSION_LZW)
      ", LZW"
#endif
#if defined(COMPRESSION_LZMA)
      ", LZMA"
#endif
#if defined(COMPRESSION_PACKBITS)
      ", RLE"
#endif
#if defined(COMPRESSION_ADOBE_DEFLATE)
      ", ZIP"
#endif
#if defined(COMPRESSION_ZSTD)
      ", ZSTD"
#endif
#if defined(COMPRESSION_WEBP)
      ", WEBP"
#endif
    ;

#if defined(MAGICKCORE_TIFF_DELEGATE)
  if (tiff_semaphore == (SemaphoreInfo *) NULL)
    ActivateSemaphoreInfo(&tiff_semaphore);
  LockSemaphoreInfo(tiff_semaphore);
  if (instantiate_key == MagickFalse)
    {
      if (CreateMagickThreadKey(&tiff_exception,NULL) == MagickFalse)
        ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
      error_handler=TIFFSetErrorHandler(TIFFErrors);
      warning_handler=TIFFSetWarningHandler(TIFFWarnings);
      if (tag_extender == (TIFFExtendProc) NULL)
        tag_extender=TIFFSetTagExtender(TIFFTagExtender);
      instantiate_key=MagickTrue;
    }
  UnlockSemaphoreInfo(tiff_semaphore);
#endif
  *version='\0';
#if defined(TIFF_VERSION)
  (void) FormatLocaleString(version,MagickPathExtent,"%d",TIFF_VERSION);
#endif
#if defined(MAGICKCORE_TIFF_DELEGATE)
  {
    const char
      *p;

    ssize_t
      i;

    p=TIFFGetVersion();
    for (i=0; (i < (MagickPathExtent-1)) && (*p != 0) && (*p != '\n'); i++)
      version[i]=(*p++);
    version[i]='\0';
  }
#endif