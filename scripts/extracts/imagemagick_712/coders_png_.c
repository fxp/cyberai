// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 71/94



            if (LocaleCompare(name,"exif") == 0)
              {
                   /* Do not write hex-encoded ICC chunk; we will
                      write it later as an eXIf chunk */
                   name=GetNextImageProfile(image);
                   continue;
              }

              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "  Setting up zTXt chunk with uuencoded %s profile",
                 name);
              Magick_png_write_raw_profile(image_info,ping,ping_info,
                (unsigned char *) name,(unsigned char *) name,
                GetStringInfoDatum(profile),(png_uint_32)
                GetStringInfoLength(profile),exception);
          }
        name=GetNextImageProfile(image);
      }
    }
  }

#if defined(PNG_WRITE_sRGB_SUPPORTED)
  if ((mng_info->have_global_srgb == MagickFalse) &&
      ping_have_iCCP != MagickTrue &&
      (ping_have_sRGB != MagickFalse ||
      png_get_valid(ping,ping_info,PNG_INFO_sRGB)))
    {
      if (mng_info->exclude_sRGB == MagickFalse)
        {
          /*
            Note image rendering intent.
          */
          if (logging != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "  Setting up sRGB chunk");

          (void) png_set_sRGB(ping,ping_info,(
            Magick_RenderingIntent_to_PNG_RenderingIntent(
              image->rendering_intent)));

          ping_have_sRGB = MagickTrue;
        }
    }

  if ((mng_info->write_mng == MagickFalse) || (!png_get_valid(ping,ping_info,PNG_INFO_sRGB)))
#endif
    {
      if (mng_info->exclude_gAMA == MagickFalse &&
          ping_have_iCCP == MagickFalse &&
          ping_have_sRGB != MagickFalse &&
          (mng_info->exclude_sRGB == MagickFalse ||
          (image->gamma < .45 || image->gamma > .46)))
      {
      if ((mng_info->have_global_gama == MagickFalse) && (image->gamma != 0.0))
        {
          /*
            Note image gamma.
          */
          if (logging != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Setting up gAMA chunk (%.20g)",image->gamma);

          png_set_gAMA(ping,ping_info,image->gamma);
        }
      }

      if (mng_info->exclude_cHRM == MagickFalse && ping_have_sRGB == MagickFalse)
        {
          if ((mng_info->have_global_chrm == MagickFalse) &&
              (image->chromaticity.red_primary.x != 0.0))
            {
              /*
                Note image chromaticity.
              */
               PrimaryInfo
                 bp,
                 gp,
                 rp,
                 wp;

               wp=image->chromaticity.white_point;
               rp=image->chromaticity.red_primary;
               gp=image->chromaticity.green_primary;
               bp=image->chromaticity.blue_primary;

               if (logging != MagickFalse)
                 (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "  Setting up cHRM chunk");

               png_set_cHRM(ping,ping_info,wp.x,wp.y,rp.x,rp.y,gp.x,gp.y,
                   bp.x,bp.y);
           }
        }
    }

  if (mng_info->exclude_bKGD == MagickFalse)
    {
      if (ping_have_bKGD != MagickFalse)
        {
          png_set_bKGD(ping,ping_info,&ping_background);
          if (logging != MagickFalse)
            {
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "    Setting up bKGD chunk");
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "      background color = (%d,%d,%d)",
                        (int) ping_background.red,
                        (int) ping_background.green,
                        (int) ping_background.blue);
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "      index = %d, gray=%d",
                        (int) ping_background.index,
                        (int) ping_background.gray);
            }
         }
    }

  if (mng_info->exclude_pHYs == MagickFalse)
    {
      if (ping_have_pHYs != MagickFalse)
        {
          png_set_pHYs(ping,ping_info,
             ping_pHYs_x_resolution,
             ping_pHYs_y_resolution,
             ping_pHYs_unit_type);

          if (logging != MagickFalse)
            {
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "    Setting up pHYs chunk");
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "      x_resolution=%g",(double) ping_pHYs_x_resolution);
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "      y_resolution=%g",(double) ping_pHYs_y_resolution);
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "      unit_type=%g",(double) ping_pHYs_unit_type);
            }
        }
    }

#if defined(PNG_tIME_SUPPORTED)
  if (mng_info->exclude_tIME == MagickFalse)
    {
      const char
        *timestamp;