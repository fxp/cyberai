// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 7/24

dule(),
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
        if ((bmp_info.height < 0) &&
          (bmp_info.compression != BI_RGB) && (bmp_info.compression != BI_BITFIELDS))
          ThrowReaderException(CoderError,"CompressNotSupported");
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
        if (bmp_info.size > 40)
          {
            double
              gamma;

            /*
              Read color management information.
            */
            bmp_info.alpha_mask=ReadBlobLSBLong(image);
            bmp_info.colorspace=ReadBlobLSBSignedLong(image);
            /*
              Decode 2^30 fixed point formatted CIE primaries.
            */
#           define BMP_DENOM ((double) 0x40000000)
            bmp_info.red_primary.x=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.red_primary.y=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.red_primary.z=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.green_primary.x=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.green_primary.y=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.green_primary.z=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.blue_primary.x=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.blue_primary.y=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.blue_primary.z=(double) ReadBlobLSBLong(image)/BMP_DENOM;

            gamma=bmp_info.red_primary.x+bmp_info.red_primary.y+
              bmp_info.red_primary.z;
            gamma=MagickSafeReciprocal(gamma);
            bmp_info.red_primary.x*=gamma;
            bmp_info.red_primary.y*=gamma;

            gamma=bmp_info.green_primary.x+bmp_info.green_primary.y+
              bmp_info.green_primary.z;
            gamma=MagickSafeReciprocal(gamma);
            bmp_info.green_primary.x*=gamma;
            bmp_info.green_primary.y*=gamma;

            gamma=bmp_info.blue_primary.x+bmp_info.blue_primary.y+
              bmp_info.blue_primary.z;
            gamma=MagickSafeReciprocal(gamma);
            bmp_info.blue_primary.x*=gamma;
            bmp_info.blue_primary.y*=gamma;

            /*
              Decode 16^16 fixed point formatted gamma_scales.
            */
            bmp_info.gamma_scale.x=(double) ReadBlobLSBLong(image)/0x10000;
            bmp_info.gamma_scale.y=(double) ReadBlobLSBLong(image)/0x10000;
            bmp_info.gamma_scale.z=(double) ReadBlobLSBLong(image)/0x10000;