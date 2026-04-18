// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 80/94



      else if (LocaleCompare(value,"4") == 0)
        mng_info->colortype = 5;

      else if (LocaleCompare(value,"6") == 0)
        mng_info->colortype = 7;

      else
        (void) ThrowMagickException(exception,GetMagickModule(),CoderWarning,
          "ignoring invalid defined png:color-type","=%s",value);

      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "  png:color-type=%d was defined.\n",
          mng_info->colortype-1);
    }

  /* Check for chunks to be excluded:
   *
   * The default is to not exclude any known chunks except for any
   * listed in the "unused_chunks" array, above.
   *
   * Chunks can be listed for exclusion via a "png:exclude-chunk"
   * define (in the image properties or in the image artifacts)
   * or via a mng_info member.  For convenience, in addition
   * to or instead of a comma-separated list of chunks, the
   * "exclude-chunk" string can be simply "all" or "none".
   *
   * Note that the "-strip" option provides a convenient way of
   * doing the equivalent of
   *
   *    -define png:exclude-chunk="bKGD,caNv,cHRM,eXIf,gAMA,iCCP,
   *            iTXt,pHYs,sRGB,tEXt,zCCP,zTXt,date"
   *
   * The exclude-chunk define takes priority over the mng_info.
   *
   * A "png:include-chunk" define takes  priority over both the
   * mng_info and the "png:exclude-chunk" define.  Like the
   * "exclude-chunk" string, it can define "all" or "none" as
   * well as a comma-separated list.  Chunks that are unknown to
   * ImageMagick are always excluded, regardless of their "copy-safe"
   * status according to the PNG specification, and even if they
   * appear in the "include-chunk" list. Such defines appearing among
   * the image options take priority over those found among the image
   * artifacts.
   *
   * Finally, all chunks listed in the "unused_chunks" array are
   * automatically excluded, regardless of the other instructions
   * or lack thereof.
   *
   * if you exclude sRGB but not gAMA (recommended), then sRGB chunk
   * will not be written and the gAMA chunk will only be written if it
   * is not between .45 and .46, or approximately (1.0/2.2).
   *
   * If you exclude tRNS and the image has transparency, the colortype
   * is forced to be 4 or 6 (GRAY_ALPHA or RGB_ALPHA).
   *
   * The -strip option causes StripImage() to set the png:include-chunk
   * artifact to "none,trns,gama".
   */

  mng_info->exclude_bKGD=MagickFalse;
  mng_info->exclude_caNv=MagickFalse;
  mng_info->exclude_cHRM=MagickFalse;
  mng_info->exclude_date=MagickFalse;
  mng_info->exclude_eXIf=MagickFalse;
  mng_info->exclude_EXIF=MagickFalse;
  mng_info->exclude_gAMA=MagickFalse;
  mng_info->exclude_iCCP=MagickFalse;
  mng_info->exclude_oFFs=MagickFalse;
  mng_info->exclude_pHYs=MagickFalse;
  mng_info->exclude_sRGB=MagickFalse;
  mng_info->exclude_tEXt=MagickFalse;
  mng_info->exclude_tIME=MagickFalse;
  mng_info->exclude_tRNS=MagickFalse;
  mng_info->exclude_zCCP=MagickFalse;
  mng_info->exclude_zTXt=MagickFalse;

  mng_info->preserve_colormap=MagickFalse;

  value=GetImageOption(image_info,"png:preserve-colormap");
  if (value == NULL)
     value=GetImageArtifact(image,"png:preserve-colormap");
  if (value != NULL)
     mng_info->preserve_colormap=MagickTrue;

  mng_info->preserve_iCCP=MagickFalse;

  value=GetImageOption(image_info,"png:preserve-iCCP");
  if (value == NULL)
     value=GetImageArtifact(image,"png:preserve-iCCP");
  if (value != NULL)
     mng_info->preserve_iCCP=MagickTrue;

  /* These compression-level, compression-strategy, and compression-filter
   * defines take precedence over values from the -quality option.
   */
  value=GetImageOption(image_info,"png:compression-level");
  if (value == NULL)
     value=GetImageArtifact(image,"png:compression-level");
  if (value != NULL)
  {
      /* We have to add 1 to everything because 0 is a valid input,
       * and we want to use 0 (the default) to mean undefined.
       */
      if (LocaleCompare(value,"0") == 0)
        mng_info->compression_level = 1;

      else if (LocaleCompare(value,"1") == 0)
        mng_info->compression_level = 2;

      else if (LocaleCompare(value,"2") == 0)
        mng_info->compression_level = 3;

      else if (LocaleCompare(value,"3") == 0)
        mng_info->compression_level = 4;

      else if (LocaleCompare(value,"4") == 0)
        mng_info->compression_level = 5;

      else if (LocaleCompare(value,"5") == 0)
        mng_info->compression_level = 6;

      else if (LocaleCompare(value,"6") == 0)
        mng_info->compression_level = 7;

      else if (LocaleCompare(value,"7") == 0)
        mng_info->compression_level = 8;

      else if (LocaleCompare(value,"8") == 0)
        mng_info->compression_level = 9;

      else if (LocaleCompare(value,"9") == 0)
        mng_info->compression_level = 10;