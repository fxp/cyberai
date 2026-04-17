/* ===== EXTRACT: bmp.c ===== */
/* Function: ReadBMPImage (part B) */
/* Lines: 750–857 */

    else
      {
        /*
          Microsoft Windows BMP image file.
        */
        switch (bmp_info.size)
        {
          case 40: case 52: case 56: case 64: case 78: case 108: case 124:
          {
            break;
          }
          default:
          {
            if (bmp_info.size < 64)
              ThrowReaderException(CorruptImageError,"ImproperImageHeader");
            break;
          }
        }
        bmp_info.width=(ssize_t) ReadBlobLSBSignedLong(image);
        bmp_info.height=(ssize_t) ReadBlobLSBSignedLong(image);
        bmp_info.planes=ReadBlobLSBShort(image);
        bmp_info.bits_per_pixel=ReadBlobLSBShort(image);
        bmp_info.compression=ReadBlobLSBLong(image);
        if (bmp_info.size > 16)
          {
            bmp_info.image_size=ReadBlobLSBLong(image);
            bmp_info.x_pixels=ReadBlobLSBLong(image);
            bmp_info.y_pixels=ReadBlobLSBLong(image);
            bmp_info.number_colors=ReadBlobLSBLong(image);
            bmp_info.colors_important=ReadBlobLSBLong(image);
          }
        if (image->debug != MagickFalse)
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Format: MS Windows bitmap");
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Geometry: %.20gx%.20g",(double) bmp_info.width,(double)
              bmp_info.height);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Bits per pixel: %.20g",(double) bmp_info.bits_per_pixel);
            switch (bmp_info.compression)
            {
              case BI_RGB:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_RGB");
                break;
              }
              case BI_RLE4:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_RLE4");
                break;
              }
              case BI_RLE8:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_RLE8");
                break;
              }
              case BI_BITFIELDS:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_BITFIELDS");
                break;
              }
              case BI_ALPHABITFIELDS:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_ALPHABITFIELDS");
                break;
              }
              case BI_PNG:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_PNG");
                break;
              }
              case BI_JPEG:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_JPEG");
                break;
              }
              default:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: UNKNOWN (%u)",bmp_info.compression);
              }
            }
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Number of colors: %u",bmp_info.number_colors);
          }
        if ((bmp_info.size > 40) || (bmp_info.compression == BI_BITFIELDS) ||
            (bmp_info.compression == BI_ALPHABITFIELDS))

          {
            bmp_info.red_mask=ReadBlobLSBLong(image);
            bmp_info.green_mask=ReadBlobLSBLong(image);
            bmp_info.blue_mask=ReadBlobLSBLong(image);
            if (bmp_info.compression == BI_ALPHABITFIELDS)
              bmp_info.alpha_mask=ReadBlobLSBLong(image);
            if (((bmp_info.size == 40) ||
                 (bmp_info.compression == BI_ALPHABITFIELDS)) &&
                 (bmp_info.bits_per_pixel != 16) &&
                 (bmp_info.bits_per_pixel != 32))
              ThrowReaderException(CorruptImageError,"UnsupportedBitsPerPixel");
          }
