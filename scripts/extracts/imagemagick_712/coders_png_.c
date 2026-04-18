// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 83/94



  if (logging != MagickFalse)
  {
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  Chunks to be excluded from the output png:");
    if (mng_info->exclude_bKGD != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    bKGD");
    if (mng_info->exclude_caNv != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    caNv");
    if (mng_info->exclude_cHRM != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    cHRM");
    if (mng_info->exclude_date != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    date");
    if (mng_info->exclude_EXIF != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    EXIF");
    if (mng_info->exclude_eXIf != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    eXIf");
    if (mng_info->exclude_gAMA != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    gAMA");
    if (mng_info->exclude_iCCP != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    iCCP");
    if (mng_info->exclude_oFFs != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    oFFs");
    if (mng_info->exclude_pHYs != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    pHYs");
    if (mng_info->exclude_sRGB != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    sRGB");
    if (mng_info->exclude_tEXt != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    tEXt");
    if (mng_info->exclude_tIME != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    tIME");
    if (mng_info->exclude_tRNS != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    tRNS");
    if (mng_info->exclude_zCCP != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    zCCP");
    if (mng_info->exclude_zTXt != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    zTXt");
  }

  mng_info->need_blob = MagickTrue;

  status=WriteOnePNGImage(mng_info,image_info,image,exception);

  mng_info=(MngWriteInfo *) RelinquishMagickMemory(mng_info);

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"exit WritePNGImage()");

  return(status);
}

#if defined(JNG_SUPPORTED)

/* Write one JNG image */
static MagickBooleanType WriteOneJNGImage(MngWriteInfo *mng_info,
   const ImageInfo *image_info,Image *image,ExceptionInfo *exception)
{
  Image
    *jpeg_image;

  ImageInfo
    *jpeg_image_info;

  MagickBooleanType
    logging = MagickFalse,
    status;

  size_t
    length;

  unsigned char
    *blob,
    chunk[80],
    *p;

  unsigned int
    jng_alpha_compression_method,
    jng_alpha_sample_depth,
    jng_color_type,
    transparent;

  size_t
    jng_alpha_quality,
    jng_quality;

  logging=LogMagickEvent(CoderEvent,GetMagickModule(),
    "  Enter WriteOneJNGImage()");

  if ((image->columns > 65500U) || (image->rows > 65500U))
    ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");

  blob=(unsigned char *) NULL;
  jpeg_image=(Image *) NULL;
  jpeg_image_info=(ImageInfo *) NULL;
  length=0;

  status=MagickTrue;
  transparent=image_info->type==GrayscaleAlphaType ||
     image_info->type==TrueColorAlphaType ||
     image->alpha_trait != UndefinedPixelTrait;

  jng_alpha_sample_depth = 0;

  jng_quality=image_info->quality == 0UL ? 75UL : image_info->quality%1000;

  jng_alpha_compression_method=(image->compression==JPEGCompression ||
    image_info->compression==JPEGCompression) ? 8 : 0;

  jng_alpha_quality=image_info->quality == 0UL ? 75UL :
      image_info->quality;

  if (jng_alpha_quality >= 1000)
    jng_alpha_quality /= 1000;

  length=0;

  if (transparent != 0)
    {
      jng_color_type=14;

      /* Create JPEG blob, image, and image_info */
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "  Creating jpeg_image_info for alpha.");

      jpeg_image_info=(ImageInfo *) CloneImageInfo(image_info);

      if (jpeg_image_info == (ImageInfo *) NULL)
        {
          jpeg_image_info=DestroyImageInfo(jpeg_image_info);
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        }

      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "  Creating jpeg_image.");