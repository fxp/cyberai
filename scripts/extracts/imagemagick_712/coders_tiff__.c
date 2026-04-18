// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 31/34



            jpeg_tables_mode=(int) StringToUnsignedLong(option);
            if (jpeg_tables_mode >= 0 && jpeg_tables_mode <= 3)
              (void) TIFFSetField(tiff,TIFFTAG_JPEGTABLESMODE,jpeg_tables_mode);
          }
#endif
        break;
      }
      case COMPRESSION_ADOBE_DEFLATE:
      {
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_BITSPERSAMPLE,
          &bits_per_sample,sans);
        if (((photometric == PHOTOMETRIC_RGB) ||
             (photometric == PHOTOMETRIC_SEPARATED) ||
             (photometric == PHOTOMETRIC_MINISBLACK)) &&
            ((bits_per_sample == 8) || (bits_per_sample == 16)))
          predictor=PREDICTOR_HORIZONTAL;
        (void) TIFFSetField(tiff,TIFFTAG_ZIPQUALITY,(long) (
          image_info->quality == UndefinedCompressionQuality ? 7 :
          MagickMin((ssize_t) image_info->quality/10,9)));
        break;
      }
      case COMPRESSION_CCITTFAX3:
      {
        /*
          Byte-aligned EOL.
        */
        (void) TIFFSetField(tiff,TIFFTAG_GROUP3OPTIONS,4);
        break;
      }
      case COMPRESSION_CCITTFAX4:
        break;
#if defined(LERC_SUPPORT) && defined(COMPRESSION_LERC)
      case COMPRESSION_LERC:
        break;
#endif
#if defined(LZMA_SUPPORT) && defined(COMPRESSION_LZMA)
      case COMPRESSION_LZMA:
      {
        if (((photometric == PHOTOMETRIC_RGB) ||
             (photometric == PHOTOMETRIC_SEPARATED) ||
             (photometric == PHOTOMETRIC_MINISBLACK)) &&
            ((bits_per_sample == 8) || (bits_per_sample == 16)))
          predictor=PREDICTOR_HORIZONTAL;
        (void) TIFFSetField(tiff,TIFFTAG_LZMAPRESET,(long) (
          image_info->quality == UndefinedCompressionQuality ? 7 :
          MagickMin((ssize_t) image_info->quality/10,9)));
        break;
      }
#endif
      case COMPRESSION_LZW:
      {
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_BITSPERSAMPLE,
          &bits_per_sample,sans);
        if (((photometric == PHOTOMETRIC_RGB) ||
             (photometric == PHOTOMETRIC_SEPARATED) ||
             (photometric == PHOTOMETRIC_MINISBLACK)) &&
            ((bits_per_sample == 8) || (bits_per_sample == 16)))
          predictor=PREDICTOR_HORIZONTAL;
        break;
      }
#if defined(WEBP_SUPPORT) && defined(COMPRESSION_WEBP)
      case COMPRESSION_WEBP:
      {
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_BITSPERSAMPLE,
          &bits_per_sample,sans);
        if (((photometric == PHOTOMETRIC_RGB) ||
             (photometric == PHOTOMETRIC_SEPARATED) ||
             (photometric == PHOTOMETRIC_MINISBLACK)) &&
            ((bits_per_sample == 8) || (bits_per_sample == 16)))
          predictor=PREDICTOR_HORIZONTAL;
        (void) TIFFSetField(tiff,TIFFTAG_WEBP_LEVEL,image_info->quality);
        if (image_info->quality >= 100)
          (void) TIFFSetField(tiff,TIFFTAG_WEBP_LOSSLESS,1);
        break;
      }
#endif
#if defined(ZSTD_SUPPORT) && defined(COMPRESSION_ZSTD)
      case COMPRESSION_ZSTD:
      {
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_BITSPERSAMPLE,
          &bits_per_sample,sans);
        if (((photometric == PHOTOMETRIC_RGB) ||
             (photometric == PHOTOMETRIC_SEPARATED) ||
             (photometric == PHOTOMETRIC_MINISBLACK)) &&
            ((bits_per_sample == 8) || (bits_per_sample == 16)))
          predictor=PREDICTOR_HORIZONTAL;
        (void) TIFFSetField(tiff,TIFFTAG_ZSTD_LEVEL,22*image_info->quality/
          100.0);
        break;
      }
#endif
      default:
        break;
    }
    if ((compress_tag == COMPRESSION_LZW) ||
        (compress_tag == COMPRESSION_ADOBE_DEFLATE))
      {
        if (quantum_info->format == FloatingPointQuantumFormat)
          predictor=PREDICTOR_FLOATINGPOINT;
        option=GetImageOption(image_info,"tiff:predictor");
        if (option != (const char * ) NULL)
          predictor=(uint16) strtol(option,(char **) NULL,10);
        if (predictor != 0)
          (void) TIFFSetField(tiff,TIFFTAG_PREDICTOR,predictor);
      }
    if ((image->resolution.x > 0.0) && (image->resolution.y > 0.0))
      {
        unsigned short
          units;

        ssize_t
          x_position = image->page.x,
          y_position = image->page.y;