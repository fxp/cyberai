// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 74/94



      quantum_type=RedQuantum;
      if (mng_info->is_palette != MagickFalse)
        {
          quantum_type=GrayQuantum;
          if (mng_info->colortype-1 == PNG_COLOR_TYPE_PALETTE)
            quantum_type=IndexQuantum;
        }
      (void) SetQuantumDepth(image,quantum_info,8);
      for (pass=0; pass < num_passes; pass++)
      {
        /*
          Convert PseudoClass image to a PNG monochrome image.
        */
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          if (logging != MagickFalse && y == 0)
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "    Writing row of pixels (0)");

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);

          if (p == (const Quantum *) NULL)
            break;

          (void) ExportQuantumPixels(image,(CacheView *) NULL,
            quantum_info,quantum_type,ping_pixels,exception);
          if (mng_info->colortype-1 != PNG_COLOR_TYPE_PALETTE)
            for (i=0; i < (ssize_t) image->columns; i++)
               *(ping_pixels+i)=(unsigned char) ((*(ping_pixels+i) > 127) ?
                      255 : 0);

          if (logging != MagickFalse && y == 0)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "    Writing row of pixels (1)");

          png_write_row(ping,ping_pixels);

          status=SetImageProgress(image,SaveImageTag,
              (MagickOffsetType) (pass * (ssize_t) image->rows + y),
              (MagickSizeType) num_passes * image->rows);

          if (status == MagickFalse)
            break;
        }
      }
    }

  else   /* Not Palette, Bilevel, or Opaque Monochrome */
    {
      if (((mng_info->write_png8 == MagickFalse) &&
           (mng_info->write_png24 == MagickFalse) &&
           (mng_info->write_png48 == MagickFalse) &&
           (mng_info->write_png64 == MagickFalse) &&
           (mng_info->write_png32 == MagickFalse)) &&
          ((image_matte != MagickFalse) || ((ping_bit_depth >= MAGICKCORE_QUANTUM_DEPTH))) &&
          ((mng_info->is_palette != MagickFalse)) && (ping_have_color == MagickFalse))
        {
          const Quantum
            *p;

          for (pass=0; pass < num_passes; pass++)
          {

          for (y=0; y < (ssize_t) image->rows; y++)
          {
            p=GetVirtualPixels(image,0,y,image->columns,1,exception);

            if (p == (const Quantum *) NULL)
              break;

            if (ping_color_type == PNG_COLOR_TYPE_GRAY)
              {
                if (mng_info->is_palette != MagickFalse)
                  (void) ExportQuantumPixels(image,(CacheView *) NULL,
                    quantum_info,GrayQuantum,ping_pixels,exception);

                else
                  (void) ExportQuantumPixels(image,(CacheView *) NULL,
                    quantum_info,RedQuantum,ping_pixels,exception);

                if (logging != MagickFalse && y == 0)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                       "    Writing GRAY PNG pixels (2)");
              }

            else /* PNG_COLOR_TYPE_GRAY_ALPHA */
              {
                if (logging != MagickFalse && y == 0)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                         "    Writing GRAY_ALPHA PNG pixels (2)");

                (void) ExportQuantumPixels(image,(CacheView *) NULL,
                  quantum_info,GrayAlphaQuantum,ping_pixels,exception);
              }

            if (logging != MagickFalse && y == 0)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "    Writing row of pixels (2)");

            png_write_row(ping,ping_pixels);

            status=SetImageProgress(image,SaveImageTag,
              (MagickOffsetType) (pass * (ssize_t) image->rows + y),
              (MagickSizeType) num_passes * image->rows);

            if (status == MagickFalse)
              break;
            }
          }
        }

      else
        {
          const Quantum
            *p;

          for (pass=0; pass < num_passes; pass++)
          {
            if ((image_depth > 8) ||
                (mng_info->write_png24 != MagickFalse) ||
                (mng_info->write_png32 != MagickFalse) ||
                (mng_info->write_png48 != MagickFalse) ||
                (mng_info->write_png64 != MagickFalse) ||
                ((mng_info->write_png8 == MagickFalse) &&
                 (mng_info->is_palette == MagickFalse)))
            {
              for (y=0; y < (ssize_t) image->rows; y++)
              {
                p=GetVirtualPixels(image,0,y,image->columns,1, exception);

                if (p == (const Quantum *) NULL)
                  break;