// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 68/94



            ping_trans_color.red*=0x0101;
            ping_trans_color.green*=0x0101;
            ping_trans_color.blue*=0x0101;
            ping_trans_color.gray*=0x0101;

            if (logging != MagickFalse)
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "    to (%d,%d,%d)",
                  (int) ping_trans_color.red,
                  (int) ping_trans_color.green,
                  (int) ping_trans_color.blue);
              }
          }
      }

    if (ping_bit_depth <  (int) mng_info->depth)
         ping_bit_depth =  (int) mng_info->depth;

    /*
      Adjust background and transparency samples in sub-8-bit grayscale files.
    */
    if (ping_bit_depth < 8 && ping_color_type ==
        PNG_COLOR_TYPE_GRAY)
      {
         png_uint_16
           maxval;

         size_t
           one=1;

         maxval=(png_uint_16) ((one << ping_bit_depth)-1);

         if (mng_info->exclude_bKGD == MagickFalse)
         {

         ping_background.gray=(png_uint_16) ((maxval/65535.)*
           (ScaleQuantumToShort((Quantum) (((GetPixelInfoIntensity(image,
           &image->background_color))) +.5))));

         if (logging != MagickFalse)
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "  Setting up bKGD chunk (2)");
         (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "      background_color index is %d",
             (int) ping_background.index);

         ping_have_bKGD = MagickTrue;
         }

         if (logging != MagickFalse)
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "  Scaling ping_trans_color.gray from %d",
             (int)ping_trans_color.gray);

         ping_trans_color.gray=(png_uint_16) ((maxval/255.)*(
           ping_trans_color.gray)+.5);

         if (logging != MagickFalse)
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "      to %d", (int)ping_trans_color.gray);
      }

  if (mng_info->exclude_bKGD == MagickFalse)
  {
    if (mng_info->is_palette != MagickFalse &&
        (int) ping_color_type == PNG_COLOR_TYPE_PALETTE)
      {
        /*
           Identify which colormap entry is the background color.
        */

        number_colors=image_colors;

        for (i=0; i < (ssize_t) MagickMax(1L*number_colors,1L); i++)
          if (IsPNGColorEqual(image->background_color,image->colormap[i]))
            break;

        ping_background.index=(png_byte) i;

        if (logging != MagickFalse)
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Setting up bKGD chunk with index=%d",(int) i);
          }

        if (i < (ssize_t) number_colors)
          {
            ping_have_bKGD = MagickTrue;

            if (logging != MagickFalse)
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "     background   =(%d,%d,%d)",
                        (int) ping_background.red,
                        (int) ping_background.green,
                        (int) ping_background.blue);
              }
          }

        else  /* Can't happen */
          {
            if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "      No room in PLTE to add bKGD color");
            ping_have_bKGD = MagickFalse;
          }
      }
  }

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "    PNG color type: %s (%.20g)", PngColorTypeToString((unsigned int)
      ping_color_type), (double) ping_color_type);
  /*
    Initialize compression level and filtering.
  */
  if (logging != MagickFalse)
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  Setting up deflate compression");

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    Compression buffer size: 32768");
    }

  png_set_compression_buffer_size(ping,32768L);

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "    Compression mem level: 9");

  png_set_compression_mem_level(ping, 9);

  /* Untangle the "-quality" setting:

     Undefined is 0; the default is used.
     Default is 75

     10's digit:

        0 or omitted: Use Z_HUFFMAN_ONLY strategy with the
           zlib default compression level

        1-9: the zlib compression level

     1's digit:

        0-4: the PNG filter method

        5:   libpng adaptive filtering if compression level > 5
             libpng filter type "none" if compression level <= 5
                or if image is grayscale or palette

        6:   libpng adaptive filtering

        7:   "LOCO" filtering (intrapixel differing) if writing
             a MNG, otherwise "none".  Did not work in IM-6.7.0-9
             and earlier because of a missing "else".