// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 69/94



        8:   Z_RLE strategy (or Z_HUFFMAN_ONLY if quality < 10), adaptive
             filtering. Unused prior to IM-6.7.0-10, was same as 6

        9:   Z_RLE strategy (or Z_HUFFMAN_ONLY if quality < 10), no PNG filters
             Unused prior to IM-6.7.0-10, was same as 6

    Note that using the -quality option, not all combinations of
    PNG filter type, zlib compression level, and zlib compression
    strategy are possible.  This will be addressed soon in a
    release that accommodates "-define png:compression-strategy", etc.

   */

  quality=image_info->quality == UndefinedCompressionQuality ? 75UL :
     image_info->quality;

  if (quality <= 9)
    {
      if (mng_info->compression_strategy == 0)
        mng_info->compression_strategy = Z_HUFFMAN_ONLY+1;
    }

  else if (mng_info->compression_level == 0)
    {
      int
        level;

      level=(int) MagickMin((ssize_t) quality/10,9);

      mng_info->compression_level = (size_t) level+1;
    }

  if (mng_info->compression_strategy == 0)
    {
        if ((quality %10) == 8 || (quality %10) == 9)
#ifdef Z_RLE  /* Z_RLE was added to zlib-1.2.0 */
          mng_info->compression_strategy=Z_RLE+1;
#else
          mng_info->write_png_compression_strategy = Z_DEFAULT_STRATEGY+1;
#endif
    }

  if (mng_info->compression_filter == 0)
        mng_info->compression_filter=((size_t) quality % 10) + 1;

  if (logging != MagickFalse)
    {
        if (mng_info->compression_level)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Compression level:    %d",
            (int) mng_info->compression_level-1);

        if (mng_info->compression_strategy)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Compression strategy: %d",
            (int) mng_info->compression_strategy-1);

        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "  Setting up filtering");

        if (mng_info->compression_filter == 6)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Base filter method: ADAPTIVE");
        else if (mng_info->compression_filter == 0 ||
                 mng_info->compression_filter == 1)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Base filter method: NONE");
        else
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Base filter method: %d",
            (int) mng_info->compression_filter-1);
    }

  if (mng_info->compression_level != 0)
    png_set_compression_level(ping,(int) mng_info->compression_level-1);

  if (mng_info->compression_filter == 6)
    {
      if (((int) ping_color_type == PNG_COLOR_TYPE_GRAY) ||
         ((int) ping_color_type == PNG_COLOR_TYPE_PALETTE) ||
         (quality < 50))
        png_set_filter(ping,PNG_FILTER_TYPE_BASE,PNG_NO_FILTERS);
      else
        png_set_filter(ping,PNG_FILTER_TYPE_BASE,PNG_ALL_FILTERS);
     }
  else if (mng_info->compression_filter == 7 ||
      mng_info->compression_filter == 10)
    png_set_filter(ping,PNG_FILTER_TYPE_BASE,PNG_ALL_FILTERS);

  else if (mng_info->compression_filter == 8)
    {
#if defined(PNG_INTRAPIXEL_DIFFERENCING)
      if (mng_info->write_mng != MagickFalse)
      {
         if (((int) ping_color_type == PNG_COLOR_TYPE_RGB) ||
             ((int) ping_color_type == PNG_COLOR_TYPE_RGBA))
        ping_filter_method=PNG_INTRAPIXEL_DIFFERENCING;
      }
#endif
      png_set_filter(ping,PNG_FILTER_TYPE_BASE,PNG_NO_FILTERS);
    }

  else if (mng_info->compression_filter == 9)
    png_set_filter(ping,PNG_FILTER_TYPE_BASE,PNG_NO_FILTERS);

  else if (mng_info->compression_filter != 0)
    png_set_filter(ping,PNG_FILTER_TYPE_BASE,
       (int) mng_info->compression_filter-1);

  if (mng_info->compression_strategy != 0)
    png_set_compression_strategy(ping,
       (int) mng_info->compression_strategy-1);

  ping_interlace_method=image_info->interlace != NoInterlace;

  if (mng_info->write_mng != MagickFalse)
    png_set_sig_bytes(ping,8);

  /* Bail out if cannot meet defined png:bit-depth or png:color-type */

  if (mng_info->colortype != 0)
    {
     if (mng_info->colortype-1 == PNG_COLOR_TYPE_GRAY)
       if (ping_have_color != MagickFalse)
         {
           ping_color_type = PNG_COLOR_TYPE_RGB;

           if (ping_bit_depth < 8)
             ping_bit_depth=8;
         }

     if (mng_info->colortype-1 == PNG_COLOR_TYPE_GRAY_ALPHA)
       if (ping_have_color != MagickFalse)
         ping_color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    }