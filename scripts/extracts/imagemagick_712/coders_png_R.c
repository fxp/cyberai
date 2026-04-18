// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 50/94



  entry->magick=(IsImageFormatHandler *) IsMNG;

  if (*version != '\0')
    entry->version=ConstantString(version);

  entry->mime_type=ConstantString("video/x-mng");
  entry->note=ConstantString(MNGNote);
  (void) RegisterMagickInfo(entry);

  entry=AcquireMagickInfo("PNG","PNG","Portable Network Graphics");

#if defined(MAGICKCORE_PNG_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadPNGImage;
  entry->encoder=(EncodeImageHandler *) WritePNGImage;
#endif

  entry->magick=(IsImageFormatHandler *) IsPNG;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags^=CoderAdjoinFlag;
  entry->mime_type=ConstantString("image/png");

  if (*version != '\0')
    entry->version=ConstantString(version);

  entry->note=ConstantString(PNGNote);
  (void) RegisterMagickInfo(entry);

  entry=AcquireMagickInfo("PNG","PNG8",
    "8-bit indexed with optional binary transparency");

#if defined(MAGICKCORE_PNG_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadPNGImage;
  entry->encoder=(EncodeImageHandler *) WritePNGImage;
#endif

  entry->magick=(IsImageFormatHandler *) IsPNG;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags^=CoderAdjoinFlag;
  entry->mime_type=ConstantString("image/png");
  (void) RegisterMagickInfo(entry);

  entry=AcquireMagickInfo("PNG","PNG24",
    "opaque or binary transparent 24-bit RGB");
  *version='\0';

  if (*version != '\0')
    entry->version=ConstantString(version);

#if defined(MAGICKCORE_PNG_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadPNGImage;
  entry->encoder=(EncodeImageHandler *) WritePNGImage;
#endif

  entry->magick=(IsImageFormatHandler *) IsPNG;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags^=CoderAdjoinFlag;
  entry->mime_type=ConstantString("image/png");
  (void) RegisterMagickInfo(entry);

  entry=AcquireMagickInfo("PNG","PNG32","opaque or transparent 32-bit RGBA");

#if defined(MAGICKCORE_PNG_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadPNGImage;
  entry->encoder=(EncodeImageHandler *) WritePNGImage;
#endif

  entry->magick=(IsImageFormatHandler *) IsPNG;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags^=CoderAdjoinFlag;
  entry->mime_type=ConstantString("image/png");
  (void) RegisterMagickInfo(entry);

  entry=AcquireMagickInfo("PNG","PNG48",
    "opaque or binary transparent 48-bit RGB");

#if defined(MAGICKCORE_PNG_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadPNGImage;
  entry->encoder=(EncodeImageHandler *) WritePNGImage;
#endif

  entry->magick=(IsImageFormatHandler *) IsPNG;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags^=CoderAdjoinFlag;
  entry->mime_type=ConstantString("image/png");
  (void) RegisterMagickInfo(entry);

  entry=AcquireMagickInfo("PNG","PNG64","opaque or transparent 64-bit RGBA");

#if defined(MAGICKCORE_PNG_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadPNGImage;
  entry->encoder=(EncodeImageHandler *) WritePNGImage;
#endif

  entry->magick=(IsImageFormatHandler *) IsPNG;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags^=CoderAdjoinFlag;
  entry->mime_type=ConstantString("image/png");
  (void) RegisterMagickInfo(entry);

  entry=AcquireMagickInfo("PNG","PNG00",
    "PNG inheriting bit-depth, color-type from original, if possible");

#if defined(MAGICKCORE_PNG_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadPNGImage;
  entry->encoder=(EncodeImageHandler *) WritePNGImage;
#endif

  entry->magick=(IsImageFormatHandler *) IsPNG;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags^=CoderAdjoinFlag;
  entry->mime_type=ConstantString("image/png");
  (void) RegisterMagickInfo(entry);

  entry=AcquireMagickInfo("PNG","JNG","JPEG Network Graphics");

#if defined(JNG_SUPPORTED)
#if defined(MAGICKCORE_PNG_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadJNGImage;
  entry->encoder=(EncodeImageHandler *) WriteJNGImage;
#endif
#endif

  entry->magick=(IsImageFormatHandler *) IsJNG;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags^=CoderAdjoinFlag;
  entry->mime_type=ConstantString("image/x-jng");
  entry->note=ConstantString(JNGNote);
  (void) RegisterMagickInfo(entry);

#ifdef IMPNG_SETJMP_NOT_THREAD_SAFE
  ping_semaphore=AcquireSemaphoreInfo();
#endif