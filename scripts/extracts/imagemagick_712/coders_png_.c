// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 81/94



      else
        (void) ThrowMagickException(exception,GetMagickModule(),CoderWarning,
          "ignoring invalid defined png:compression-level","=%s",value);
    }

  value=GetImageOption(image_info,"png:compression-strategy");
  if (value == NULL)
     value=GetImageArtifact(image,"png:compression-strategy");
  if (value != NULL)
  {
      if (LocaleCompare(value,"0") == 0)
        mng_info->compression_strategy = Z_DEFAULT_STRATEGY+1;

      else if (LocaleCompare(value,"1") == 0)
        mng_info->compression_strategy = Z_FILTERED+1;

      else if (LocaleCompare(value,"2") == 0)
        mng_info->compression_strategy = Z_HUFFMAN_ONLY+1;

      else if (LocaleCompare(value,"3") == 0)
#ifdef Z_RLE  /* Z_RLE was added to zlib-1.2.0 */
        mng_info->compression_strategy = Z_RLE+1;
#else
        mng_info->write_png_compression_strategy = Z_DEFAULT_STRATEGY+1;
#endif

      else if (LocaleCompare(value,"4") == 0)
#ifdef Z_FIXED  /* Z_FIXED was added to zlib-1.2.2.2 */
        mng_info->compression_strategy = Z_FIXED+1;
#else
        mng_info->write_png_compression_strategy = Z_DEFAULT_STRATEGY+1;
#endif

      else
        (void) ThrowMagickException(exception,GetMagickModule(),CoderWarning,
          "ignoring invalid defined png:compression-strategy","=%s",value);
    }

  value=GetImageOption(image_info,"png:compression-filter");
  if (value == NULL)
     value=GetImageArtifact(image,"png:compression-filter");
  if (value != NULL)
  {
      /* To do: combinations of filters allowed by libpng
       * masks 0x08 through 0xf8
       *
       * Implement this as a comma-separated list of 0,1,2,3,4,5
       * where 5 is a special case meaning PNG_ALL_FILTERS.
       */

      mng_info->compression_filter = StringToUnsignedLong(value)+1;
      if (mng_info->compression_filter > 6)
        (void) ThrowMagickException(exception,GetMagickModule(),CoderWarning,
          "ignoring invalid defined png:compression-filter","=%s",value);
  }

  for (source=0; source<8; source++)
  {
    value = (const char *) NULL;

    switch(source)
    {
      case 0:
        value=GetImageOption(image_info,"png:exclude-chunks");
        break;
      case 1:
        value=GetImageArtifact(image,"png:exclude-chunks");
        break;
      case 2:
        value=GetImageOption(image_info,"png:exclude-chunk");
        break;
      case 3:
        value=GetImageArtifact(image,"png:exclude-chunk");
        break;
      case 4:
        value=GetImageOption(image_info,"png:include-chunks");
        break;
      case 5:
        value=GetImageArtifact(image,"png:include-chunks");
        break;
      case 6:
        value=GetImageOption(image_info,"png:include-chunk");
        break;
      case 7:
        value=GetImageArtifact(image,"png:include-chunk");
        break;
    }

    if (value == NULL)
       continue;

    if (source < 4)
      excluding = MagickTrue;
    else
      excluding = MagickFalse;

    if (logging != MagickFalse)
      {
        if (source == 0 || source == 2)
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  png:exclude-chunk=%s found in image options.\n", value);
        else if (source == 1 || source == 3)
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  png:exclude-chunk=%s found in image artifacts.\n", value);
        else if (source == 4 || source == 6)
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  png:include-chunk=%s found in image options.\n", value);
        else /* if (source == 5 || source == 7) */
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  png:include-chunk=%s found in image artifacts.\n", value);
      }

    if (IsOptionMember("all",value) != MagickFalse)
      {
        mng_info->exclude_bKGD=excluding;
        mng_info->exclude_caNv=excluding;
        mng_info->exclude_cHRM=excluding;
        mng_info->exclude_date=excluding;
        mng_info->exclude_EXIF=excluding;
        mng_info->exclude_eXIf=excluding;
        mng_info->exclude_gAMA=excluding;
        mng_info->exclude_iCCP=excluding;
        mng_info->exclude_oFFs=excluding;
        mng_info->exclude_pHYs=excluding;
        mng_info->exclude_sRGB=excluding;
        mng_info->exclude_tEXt=excluding;
        mng_info->exclude_tIME=excluding;
        mng_info->exclude_tRNS=excluding;
        mng_info->exclude_zCCP=excluding;
        mng_info->exclude_zTXt=excluding;
      }