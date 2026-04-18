// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 66/94



           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Tentative PNG bit depth: %d",ping_bit_depth);
         }

      if (ping_bit_depth < (int) mng_info->depth)
         ping_bit_depth = (int) mng_info->depth;
    }
  (void) old_bit_depth;
  image_depth=(size_t) ping_bit_depth;

  if (logging != MagickFalse)
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    Tentative PNG color type: %s (%.20g)",
        PngColorTypeToString((unsigned int) ping_color_type),
        (double) ping_color_type);

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    image_info->type: %.20g",(double) image_info->type);

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    image_depth: %.20g",(double) image_depth);

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),

        "    image->depth: %.20g",(double) image->depth);

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    ping_bit_depth: %.20g",(double) ping_bit_depth);
    }

  if (matte != MagickFalse)
    {
      if (mng_info->is_palette != MagickFalse)
        {
          if (mng_info->colortype == 0)
            {
              ping_color_type=PNG_COLOR_TYPE_GRAY_ALPHA;

              if (ping_have_color != MagickFalse)
                 ping_color_type=PNG_COLOR_TYPE_RGBA;
            }

          /*
           * Determine if there is any transparent color.
          */
          if (number_transparent + number_semitransparent == 0)
            {
              /*
                No transparent pixels are present.  Change 4 or 6 to 0 or 2.
              */

              image_matte=MagickFalse;

              if (mng_info->colortype == 0)
                ping_color_type&=0x03;
            }

          else
            {
              unsigned int
                mask;

              mask=0xffff;

              if (ping_bit_depth == 8)
                 mask=0x00ff;

              if (ping_bit_depth == 4)
                 mask=0x000f;

              if (ping_bit_depth == 2)
                 mask=0x0003;

              if (ping_bit_depth == 1)
                 mask=0x0001;

              ping_trans_color.red=(png_uint_16)
                (ScaleQuantumToShort((Quantum) image->colormap[0].red) & mask);

              ping_trans_color.green=(png_uint_16)
                (ScaleQuantumToShort((Quantum) image->colormap[0].green) & mask);

              ping_trans_color.blue=(png_uint_16)
                (ScaleQuantumToShort((Quantum) image->colormap[0].blue) & mask);

              ping_trans_color.gray=(png_uint_16)
                (ScaleQuantumToShort((Quantum) GetPixelInfoIntensity(image,
                   image->colormap)) & mask);

              ping_trans_color.index=(png_byte) 0;

              ping_have_tRNS=MagickTrue;
            }

          if (ping_have_tRNS != MagickFalse)
            {
              /*
               * Determine if there is one and only one transparent color
               * and if so if it is fully transparent.
               */
              if (ping_have_cheap_transparency == MagickFalse)
                ping_have_tRNS=MagickFalse;
            }

          if (ping_have_tRNS != MagickFalse)
            {
              if (mng_info->colortype == 0)
                ping_color_type &= 0x03;  /* changes 4 or 6 to 0 or 2 */

              if (image_depth == 8)
                {
                  ping_trans_color.red&=0xff;
                  ping_trans_color.green&=0xff;
                  ping_trans_color.blue&=0xff;
                  ping_trans_color.gray&=0xff;
                }
            }
        }
      else
        {
          if (image_depth == 8)
            {
              ping_trans_color.red&=0xff;
              ping_trans_color.green&=0xff;
              ping_trans_color.blue&=0xff;
              ping_trans_color.gray&=0xff;
            }
        }
    }

    matte=image_matte;

    if (ping_have_tRNS != MagickFalse)
      image_matte=MagickFalse;

    if ((mng_info->is_palette != MagickFalse) &&
        mng_info->colortype-1 != PNG_COLOR_TYPE_PALETTE &&
        ping_have_color == MagickFalse &&
        (image_matte == MagickFalse || image_depth >= 8))
      {
        size_t one=1;

        if (image_matte != MagickFalse)
          ping_color_type=PNG_COLOR_TYPE_GRAY_ALPHA;

        else if (mng_info->colortype-1 != PNG_COLOR_TYPE_GRAY_ALPHA)
          {
            ping_color_type=PNG_COLOR_TYPE_GRAY;

            if (save_image_depth == 16 && image_depth == 8)
              {
                if (logging != MagickFalse)
                  {
                    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                        "  Scaling ping_trans_color (0)");
                  }
                    ping_trans_color.gray*=0x0101;
              }
          }

        if (image_depth > MAGICKCORE_QUANTUM_DEPTH)
          image_depth=MAGICKCORE_QUANTUM_DEPTH;