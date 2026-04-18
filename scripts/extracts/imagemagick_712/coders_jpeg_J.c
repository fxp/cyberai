// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 10/24



      /*
        Scale the image.
      */
      flags=ParseGeometry(option,&geometry_info);
      if ((flags & SigmaValue) == 0)
        geometry_info.sigma=geometry_info.rho;
      jpeg_calc_output_dimensions(jpeg_info);
      image->magick_columns=jpeg_info->output_width;
      image->magick_rows=jpeg_info->output_height;
      scale_factor=1.0;
      if (geometry_info.rho != 0.0)
        scale_factor=jpeg_info->output_width/geometry_info.rho;
      if ((geometry_info.sigma != 0.0) &&
          (scale_factor > (jpeg_info->output_height/geometry_info.sigma)))
        scale_factor=jpeg_info->output_height/geometry_info.sigma;
#if defined(LIBJPEG_TURBO_VERSION_NUMBER) || (JPEG_LIB_VERSION >= 70)
      jpeg_info->scale_num=(unsigned int) (8.0/scale_factor+0.5);
      if (jpeg_info->scale_num > 16U)
        jpeg_info->scale_num=16U;
      if (jpeg_info->scale_num < 1U)
        jpeg_info->scale_num=1U;
      jpeg_info->scale_denom=8U;
#else
      jpeg_info->scale_num=1U;
      jpeg_info->scale_denom=(unsigned int) scale_factor;
#endif
      jpeg_calc_output_dimensions(jpeg_info);
      if (image->debug != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Scale factor: %.20g",(double) scale_factor);
    }
#if (JPEG_LIB_VERSION >= 61) && defined(D_PROGRESSIVE_SUPPORTED)
#if !defined(LIBJPEG_TURBO_VERSION_NUMBER) && defined(D_LOSSLESS_SUPPORTED)
  image->interlace=jpeg_info->process == JPROC_PROGRESSIVE ?
    JPEGInterlace : NoInterlace;
  image->compression=jpeg_info->process == JPROC_LOSSLESS ?
    LosslessJPEGCompression : JPEGCompression;
  if (jpeg_info->data_precision > 8)
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
      "12-bit JPEG not supported. Reducing pixel data to 8 bits","`%s'",
      image->filename);
  if (jpeg_info->data_precision == 16)
    jpeg_info->data_precision=12;
#else
  image->interlace=jpeg_info->progressive_mode != 0 ? JPEGInterlace :
    NoInterlace;
  image->compression=JPEGCompression;
#endif
#else
  image->compression=JPEGCompression;
  image->interlace=JPEGInterlace;
#endif
  option=GetImageOption(image_info,"jpeg:colors");
  if (option != (const char *) NULL)
    {
      /*
        Let the JPEG library quantize the image.
      */
      jpeg_info->quantize_colors=TRUE;
      jpeg_info->desired_number_of_colors=StringToInteger(option);
    }
  option=GetImageOption(image_info,"jpeg:block-smoothing");
  if (option != (const char *) NULL)
    jpeg_info->do_block_smoothing=IsStringTrue(option) != MagickFalse ? TRUE :
      FALSE;
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
  option=GetImageOption(image_info,"jpeg:fancy-upsampling");
  if (option != (const char *) NULL)
    jpeg_info->do_fancy_upsampling=IsStringTrue(option) != MagickFalse ? TRUE :
      FALSE;
  jpeg_calc_output_dimensions(jpeg_info);
  image->columns=jpeg_info->output_width;
  image->rows=jpeg_info->output_height;
  image->depth=(size_t) jpeg_info->data_precision;
  if (IsITUFaxImage(image) != MagickFalse)
    {
      (void) SetImageColorspace(image,LabColorspace,exception);
      jpeg_info->out_color_space=JCS_YCbCr;
    }
  else
    {
      switch (jpeg_info->out_color_space)
      {
        case JCS_RGB:
        default:
        {
          (void) SetImageColorspace(image,sRGBColorspace,exception);
          break;
        }
        case JCS_GRAYSCALE:
        {
          (void) SetImageColorspace(image,GRAYColorspace,exception);
          break;
        }
        case JCS_YCbCr:
        {
          (void) SetImageColorspace(image,YCbCrColorspace,exception);
          break;
        }
        case JCS_CMYK:
        {
          (void) SetImageColorspace(image,CMYKColorspace,exception);
          break;
        }
      }
    }
  option=GetImageOption(image_info,"jpeg:colors");
  if (option != (const char *) NULL)
    if (AcquireImageColormap(image,StringToUnsignedLong(option),exception) == MagickFalse)
      {
        client_info=JPEGCleanup(jpeg_info,client_info);
        ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
      }
  if ((jpeg_info->output_components == 1) && (jpeg_info->quantize_colors == 0))
    {
      size_t
        colors;