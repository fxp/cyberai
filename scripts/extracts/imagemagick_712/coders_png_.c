// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 67/94



        if ((image_colors == 0) ||
             ((ssize_t) (image_colors-1) > (ssize_t) MaxColormapSize))
          image_colors=(int) (one << image_depth);

        if (image_depth > 8)
          ping_bit_depth=16;

        else
          {
            ping_bit_depth=8;
            if ((int) ping_color_type == PNG_COLOR_TYPE_PALETTE)
              {
                if(!mng_info->depth)
                  {
                    ping_bit_depth=1;

                    while ((int) (one << ping_bit_depth)
                        < (ssize_t) image_colors)
                      ping_bit_depth <<= 1;
                  }
              }

            else if (ping_color_type ==
                PNG_COLOR_TYPE_GRAY && image_colors < 17 &&
                mng_info->is_palette != MagickFalse)
              {
              /* Check if grayscale is reducible */

                int
                  depth_4_ok=MagickTrue,
                  depth_2_ok=MagickTrue,
                  depth_1_ok=MagickTrue;

                for (i=0; i < (ssize_t) image_colors; i++)
                {
                   unsigned char
                     intensity;

                   intensity=ScaleQuantumToChar((Quantum) image->colormap[i].red);

                   if ((intensity & 0x0f) != ((intensity & 0xf0) >> 4))
                     depth_4_ok=depth_2_ok=depth_1_ok=MagickFalse;
                   else if ((intensity & 0x03) != ((intensity & 0x0c) >> 2))
                     depth_2_ok=depth_1_ok=MagickFalse;
                   else if ((intensity & 0x01) != ((intensity & 0x02) >> 1))
                     depth_1_ok=MagickFalse;
                }

                if (depth_1_ok && mng_info->depth <= 1)
                  ping_bit_depth=1;

                else if (depth_2_ok && mng_info->depth <= 2)
                  ping_bit_depth=2;

                else if (depth_4_ok && mng_info->depth <= 4)
                  ping_bit_depth=4;
              }
          }

          image_depth=(size_t) ping_bit_depth;
      }

    else

      if (mng_info->is_palette != MagickFalse)
      {
        number_colors=image_colors;

        if (image_depth <= 8)
          {
            /*
              Set image palette.
            */
            ping_color_type=(png_byte) PNG_COLOR_TYPE_PALETTE;

            if (!(mng_info->have_global_plte != MagickFalse && matte == MagickFalse))
              {
                for (i=0; i < (ssize_t) number_colors; i++)
                {
                  palette[i].red=ScaleQuantumToChar((Quantum) image->colormap[i].red);
                  palette[i].green=ScaleQuantumToChar((Quantum) image->colormap[i].green);
                  palette[i].blue=ScaleQuantumToChar((Quantum) image->colormap[i].blue);
                }

                if (logging != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "  Setting up PLTE chunk with %d colors",
                    number_colors);

                ping_have_PLTE=MagickTrue;
              }

            /* color_type is PNG_COLOR_TYPE_PALETTE */
            if (mng_info->depth == 0)
              {
                size_t
                  one;

                ping_bit_depth=1;
                one=1;

                while ((one << ping_bit_depth) < (size_t) number_colors)
                  ping_bit_depth <<= 1;
              }

            ping_num_trans=0;

            if (matte != MagickFalse)
              {
                /*
                 * Set up trans_colors array.
                 */
                assert(number_colors <= 256);

                ping_num_trans=(unsigned short) (number_transparent +
                  number_semitransparent);

                if (ping_num_trans == 0)
                  ping_have_tRNS=MagickFalse;

                else
                  {
                    if (logging != MagickFalse)
                      {
                        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                          "  Scaling ping_trans_color (1)");
                      }
                    ping_have_tRNS=MagickTrue;

                    for (i=0; i < ping_num_trans; i++)
                    {
                       ping_trans_alpha[i]= (png_byte)
                         ScaleQuantumToChar((Quantum) image->colormap[i].alpha);
                    }
                  }
              }
          }
      }

    else
      {

        if (image_depth < 8)
          image_depth=8;

        if ((save_image_depth == 16) && (image_depth == 8))
          {
            if (logging != MagickFalse)
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "    Scaling ping_trans_color from (%d,%d,%d)",
                  (int) ping_trans_color.red,
                  (int) ping_trans_color.green,
                  (int) ping_trans_color.blue);
              }