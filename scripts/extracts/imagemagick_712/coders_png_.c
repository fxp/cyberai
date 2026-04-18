// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 65/94



      if (mng_info->exclude_bKGD == MagickFalse)
      {
       /*
        * Identify which colormap entry is the background color.
        */

        for (i=0; i < (ssize_t) MagickMax(1L*number_colors-1L,1L); i++)
          if (IsPNGColorEqual(ping_background,image->colormap[i]))
            break;

        ping_background.index=(png_byte) i;

        if (logging != MagickFalse)
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "      background_color index is %d",
                 (int) ping_background.index);
          }
      }
    } /* end of write_png8 */

  else if (mng_info->colortype == 1)
    {
      image_matte=MagickFalse;
      ping_color_type=(png_byte) PNG_COLOR_TYPE_GRAY;
    }

  else if ((mng_info->write_png24 != MagickFalse) ||
           (mng_info->write_png48 != MagickFalse) ||
           (mng_info->colortype == 3))
    {
      image_matte=MagickFalse;
      ping_color_type=(png_byte) PNG_COLOR_TYPE_RGB;
    }

  else if ((mng_info->write_png32 != MagickFalse) ||
           (mng_info->write_png64 != MagickFalse) ||
           (mng_info->colortype == 7))
    {
      image_matte=MagickTrue;
      ping_color_type=(png_byte) PNG_COLOR_TYPE_RGB_ALPHA;
    }

  else /* mng_info->write_pngNN not specified */
    {
      image_depth=(size_t) ping_bit_depth;

      if (mng_info->colortype != 0)
        {
          ping_color_type=(png_byte) mng_info->colortype-1;

          if (ping_color_type == PNG_COLOR_TYPE_GRAY_ALPHA ||
              ping_color_type == PNG_COLOR_TYPE_RGB_ALPHA)
            image_matte=MagickTrue;

          else
            image_matte=MagickFalse;

          if (logging != MagickFalse)
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "   PNG colortype %d was specified:",(int) ping_color_type);
        }

      else /* write_png_colortype not specified */
        {
          if (logging != MagickFalse)
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "  Selecting PNG colortype:");

          if (image_info->type == TrueColorType)
            {
              ping_color_type=(png_byte) PNG_COLOR_TYPE_RGB;
              image_matte=MagickFalse;
            }
          else if (image_info->type == TrueColorAlphaType)
            {
              ping_color_type=(png_byte) PNG_COLOR_TYPE_RGB_ALPHA;
              image_matte=MagickTrue;
            }
          else if (image_info->type == PaletteType ||
                   image_info->type == PaletteAlphaType)
            ping_color_type=(png_byte) PNG_COLOR_TYPE_PALETTE;
          else
            {
              if (ping_have_color == MagickFalse)
                {
                  if (image_matte == MagickFalse)
                    {
                      ping_color_type=(png_byte) PNG_COLOR_TYPE_GRAY;
                      image_matte=MagickFalse;
                    }

                  else
                    {
                      ping_color_type=(png_byte) PNG_COLOR_TYPE_GRAY_ALPHA;
                      image_matte=MagickTrue;
                    }
                }
              else
                {
                  if (image_matte == MagickFalse)
                    {
                      ping_color_type=(png_byte) PNG_COLOR_TYPE_RGB;
                      image_matte=MagickFalse;
                    }

                  else
                    {
                      ping_color_type=(png_byte) PNG_COLOR_TYPE_RGBA;
                      image_matte=MagickTrue;
                    }
                 }
            }

        }

      if (logging != MagickFalse)
         (void) LogMagickEvent(CoderEvent,GetMagickModule(),
         "    Selected PNG colortype=%d",ping_color_type);

      if (ping_bit_depth < 8)
        {
          if (ping_color_type == PNG_COLOR_TYPE_GRAY_ALPHA ||
              ping_color_type == PNG_COLOR_TYPE_RGB ||
              ping_color_type == PNG_COLOR_TYPE_RGB_ALPHA)
            ping_bit_depth=8;
        }

      old_bit_depth=(size_t) ping_bit_depth;

      if (ping_color_type == PNG_COLOR_TYPE_GRAY)
        {
          if (((image->alpha_trait & BlendPixelTrait) == 0) &&
              ((image->alpha_trait & UpdatePixelTrait) == 0) &&
               (ping_have_non_bw == MagickFalse))
             ping_bit_depth=1;
        }

      if (ping_color_type == PNG_COLOR_TYPE_PALETTE)
        {
           size_t one = 1;
           ping_bit_depth=1;

           if (image_colors == 0)
           {
              /* DO SOMETHING */
                png_error(ping,"image has 0 colors");
           }

           while ((ping_bit_depth < 64) && (int) (one << ping_bit_depth) < (ssize_t) image_colors)
             ping_bit_depth <<= 1;
        }

      if (logging != MagickFalse)
         {
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Number of colors: %.20g",(double) image_colors);