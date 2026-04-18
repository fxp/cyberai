// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 20/24


        y_density=(UINT16) image->resolution.y;

      if ((x_density > 0) && (y_density > 0))
        {
          /*
            Set image resolution.
          */
          jpeg_info->write_JFIF_header=TRUE;
          jpeg_info->X_density=x_density;
          jpeg_info->Y_density=y_density;
          /*
            Set image resolution units.
          */
          if (image->units == PixelsPerInchResolution)
            jpeg_info->density_unit=(UINT8) 1;
          else if (image->units == PixelsPerCentimeterResolution)
            jpeg_info->density_unit=(UINT8) 2;
        }
    }
  dct_method=GetImageOption(image_info,"jpeg:dct-method");
  if (dct_method != (const char *) NULL)
    switch (*dct_method)
    {
      case 'D':
      case 'd':
      {
        if (LocaleCompare(dct_method,"default") == 0)
          jpeg_info->dct_method=JDCT_DEFAULT;
        break;
      }
      case 'F':
      case 'f':
      {
        if (LocaleCompare(dct_method,"fastest") == 0)
          jpeg_info->dct_method=JDCT_FASTEST;
        if (LocaleCompare(dct_method,"float") == 0)
          jpeg_info->dct_method=JDCT_FLOAT;
        break;
      }
      case 'I':
      case 'i':
      {
        if (LocaleCompare(dct_method,"ifast") == 0)
          jpeg_info->dct_method=JDCT_IFAST;
        if (LocaleCompare(dct_method,"islow") == 0)
          jpeg_info->dct_method=JDCT_ISLOW;
        break;
      }
    }
  option=GetImageOption(image_info,"jpeg:optimize-coding");
  if (option != (const char *) NULL)
    jpeg_info->optimize_coding=IsStringTrue(option) != MagickFalse ? TRUE :
      FALSE;
  else
    {
      MagickSizeType
        length;

      length=(MagickSizeType) jpeg_info->input_components*image->columns*
        image->rows*sizeof(JSAMPLE);
      if (length == (MagickSizeType) ((size_t) length))
        {
          /*
            Perform optimization only if available memory resources permit it.
          */
          status=AcquireMagickResource(MemoryResource,length);
          if (status != MagickFalse)
            RelinquishMagickResource(MemoryResource,length);
          jpeg_info->optimize_coding=status == MagickFalse ? FALSE : TRUE;
        }
    }
#if defined(C_ARITH_CODING_SUPPORTED)
  option=GetImageOption(image_info,"jpeg:arithmetic-coding");
  if (IsStringTrue(option) != MagickFalse)
    {
      jpeg_info->arith_code=TRUE;
      jpeg_info->optimize_coding=FALSE;  /* not supported */
    }
#endif
#if (JPEG_LIB_VERSION >= 61) && defined(C_PROGRESSIVE_SUPPORTED)
  if ((LocaleCompare(image_info->magick,"PJPEG") == 0) ||
      (image_info->interlace != NoInterlace))
    {
      if (image->debug != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Interlace: progressive");
      jpeg_simple_progression(jpeg_info);
    }
  else
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "Interlace: non-progressive");
#else
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "Interlace: nonprogressive");
#endif
  quality=92;
  if ((image_info->compression != LosslessJPEGCompression) &&
      (image->quality <= 100))
    {
      if (image->quality != UndefinedCompressionQuality)
        quality=(int) image->quality;
      if (image->debug != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Quality: %.20g",
          (double) image->quality);
    }
  else
    {
#if defined(C_LOSSLESS_SUPPORTED)
      if (image->quality < 100)
        (void) ThrowMagickException(exception,GetMagickModule(),CoderWarning,
          "LosslessToLossyJPEGConversion","`%s'",image->filename);
      else
        {
          int
            point_transform,
            predictor;

          predictor=(int) image->quality/100;  /* range 1-7 */
          point_transform=image->quality % 20;  /* range 0-15 */
#if defined(MAGICKCORE_HAVE_JPEG_SIMPLE_LOSSLESS)
          jpeg_simple_lossless(jpeg_info,predictor,point_transform);
#endif
#if defined(MAGICKCORE_HAVE_JPEG_ENABLE_LOSSLESS)
          jpeg_enable_lossless(jpeg_info,predictor,point_transform);
#endif
          if (image->debug != MagickFalse)
            {
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "Compression: lossless");
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "Predictor: %d",predictor);
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "Point Transform: %d",point_transform);
            }
        }
#else
      quality=100;
      if (image->debug != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Quality: 100");
#endif
    }
  option=GetImageOption(image_info,"jpeg:extent");
  if (option != (const char *) NULL)
    {
      Image
        *jpeg_image;

      ImageInfo
        *extent_info;