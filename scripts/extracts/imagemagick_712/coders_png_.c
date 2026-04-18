// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 70/94



  if (ping_need_colortype_warning != MagickFalse ||
     ((mng_info->depth &&
     (int) mng_info->depth != ping_bit_depth) ||
     (mng_info->colortype &&
     ((int) mng_info->colortype-1 != ping_color_type &&
      mng_info->colortype != 7 &&
      !(mng_info->colortype == 5 && ping_color_type == 0)))))
    {
      if (logging != MagickFalse)
        {
          if (ping_need_colortype_warning != MagickFalse)
            {
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "  Image has transparency but tRNS chunk was excluded");
            }

          if (mng_info->depth)
            {
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Defined png:bit-depth=%u, Computed depth=%u",
                  mng_info->depth,
                  ping_bit_depth);
            }

          if (mng_info->colortype)
            {
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Defined png:color-type=%u, Computed color type=%u",
                  mng_info->colortype-1,
                  ping_color_type);
            }
        }

      png_warning(ping,
        "Cannot write image with defined png:bit-depth or png:color-type.");
    }

  if ((image_matte != MagickFalse) &&
      ((image->alpha_trait & BlendPixelTrait) == 0) &&
      ((image->alpha_trait & UpdatePixelTrait) == 0))
    {
      /* Add an opaque matte channel */
      image->alpha_trait = BlendPixelTrait;
      (void) SetImageAlpha(image,OpaqueAlpha,exception);

      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "  Added an opaque matte channel");
    }

  if (number_transparent != 0 || number_semitransparent != 0)
    {
      if (ping_color_type < 4)
        {
           ping_have_tRNS=MagickTrue;
           if (logging != MagickFalse)
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
               "  Setting ping_have_tRNS=MagickTrue.");
        }
    }

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  Writing PNG header chunks");

  png_set_IHDR(ping,ping_info,ping_width,ping_height,
               ping_bit_depth,ping_color_type,
               ping_interlace_method,ping_compression_method,
               ping_filter_method);

  if (ping_color_type == 3 && ping_have_PLTE != MagickFalse)
    {
      png_set_PLTE(ping,ping_info,palette,number_colors);

      if (logging != MagickFalse)
        {
          for (i=0; i< (ssize_t) number_colors; i++)
          {
            if (i < ping_num_trans)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "     PLTE[%d] = (%d,%d,%d), tRNS[%d] = (%d)",
                      (int) i,
                      (int) palette[i].red,
                      (int) palette[i].green,
                      (int) palette[i].blue,
                      (int) i,
                      (int) ping_trans_alpha[i]);
             else
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "     PLTE[%d] = (%d,%d,%d)",
                      (int) i,
                      (int) palette[i].red,
                      (int) palette[i].green,
                      (int) palette[i].blue);
           }
         }
    }

  /* Only write the iCCP chunk if we are not writing the sRGB chunk. */
  if (mng_info->exclude_sRGB != MagickFalse ||
     (!png_get_valid(ping,ping_info,PNG_INFO_sRGB)))
  {
    if ((mng_info->exclude_tEXt == MagickFalse ||
         mng_info->exclude_zTXt == MagickFalse) &&
       (ping_exclude_iCCP == MagickFalse || ping_exclude_zCCP == MagickFalse))
    {
      ResetImageProfileIterator(image);
      for (name=GetNextImageProfile(image); name != (char *) NULL; )
      {
        profile=GetImageProfile(image,name);

        if (profile != (StringInfo *) NULL)
          {
#ifdef PNG_WRITE_iCCP_SUPPORTED
            if ((LocaleCompare(name,"ICC") == 0) ||
                (LocaleCompare(name,"ICM") == 0))
              {
                ping_have_iCCP = MagickTrue;
                if (ping_exclude_iCCP == MagickFalse)
                  {
                    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                        "  Setting up iCCP chunk");

                    png_set_iCCP(ping,ping_info,(png_charp) name,0,
#if (PNG_LIBPNG_VER < 10500)
                    (png_charp) GetStringInfoDatum(profile),
#else
                    (const png_byte *) GetStringInfoDatum(profile),
#endif
                    (png_uint_32) GetStringInfoLength(profile));
                  }
                else
                  {
                    /* Do not write hex-encoded ICC chunk */
                       name=GetNextImageProfile(image);
                       continue;
                  }
              }
#endif /* WRITE_iCCP */