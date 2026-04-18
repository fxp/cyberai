// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 8/24



            if (bmp_info.colorspace == 0)
              {
                image->chromaticity.red_primary.x=bmp_info.red_primary.x;
                image->chromaticity.red_primary.y=bmp_info.red_primary.y;
                image->chromaticity.green_primary.x=bmp_info.green_primary.x;
                image->chromaticity.green_primary.y=bmp_info.green_primary.y;
                image->chromaticity.blue_primary.x=bmp_info.blue_primary.x;
                image->chromaticity.blue_primary.y=bmp_info.blue_primary.y;
                /*
                  Compute a single gamma from the BMP 3-channel gamma.
                */
                image->gamma=(bmp_info.gamma_scale.x+bmp_info.gamma_scale.y+
                  bmp_info.gamma_scale.z)/3.0;
              }
          }
        else
          (void) CopyMagickString(image->magick,"BMP3",MagickPathExtent);
        if (bmp_info.size > 108)
          {
            size_t
              intent;

            /*
              Read BMP Version 5 color management information.
            */
            intent=ReadBlobLSBLong(image);
            switch ((int) intent)
            {
              case LCS_GM_BUSINESS:
              {
                image->rendering_intent=SaturationIntent;
                break;
              }
              case LCS_GM_GRAPHICS:
              {
                image->rendering_intent=RelativeIntent;
                break;
              }
              case LCS_GM_IMAGES:
              {
                image->rendering_intent=PerceptualIntent;
                break;
              }
              case LCS_GM_ABS_COLORIMETRIC:
              {
                image->rendering_intent=AbsoluteIntent;
                break;
              }
            }
            profile_data=(MagickOffsetType) ReadBlobLSBLong(image);
            profile_size=(MagickOffsetType) ReadBlobLSBLong(image);
            (void) ReadBlobLSBLong(image);  /* Reserved byte */
          }
      }
    if ((ignore_filesize == MagickFalse) &&
        (MagickSizeType) bmp_info.file_size != blob_size)
      (void) ThrowMagickException(exception,GetMagickModule(),
        CorruptImageError,"LengthAndFilesizeDoNotMatch","`%s'",
        image->filename);
    if (bmp_info.width <= 0)
      ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize");
    if (bmp_info.height == 0)
      ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize");
    if (bmp_info.compression == BI_JPEG)
      {
        /*
          Read embedded JPEG image.
        */
        Image *embed_image = ReadEmbedImage(image_info,image,"jpeg",exception);
        (void) CloseBlob(image);
        image=DestroyImageList(image);
        return(embed_image);
      }
    if (bmp_info.compression == BI_PNG)
      {
        /*
          Read embedded PNG image.
        */
        Image *embed_image = ReadEmbedImage(image_info,image,"png",exception);
        (void) CloseBlob(image);
        image=DestroyImageList(image);
        return(embed_image);
      }
    if (bmp_info.planes != 1)
      ThrowReaderException(CorruptImageError,"StaticPlanesValueNotEqualToOne");
    switch (bmp_info.bits_per_pixel)
    {
      case 1: case 4: case 8: case 16: case 24: case 32: case 64:
      {
        break;
      }
      default:
        ThrowReaderException(CorruptImageError,"UnsupportedBitsPerPixel");
    }
    if ((bmp_info.bits_per_pixel < 16) &&
        (bmp_info.number_colors > (1U << bmp_info.bits_per_pixel)))
      ThrowReaderException(CorruptImageError,"UnrecognizedNumberOfColors");
    if ((MagickSizeType) bmp_info.number_colors > blob_size)
      ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
    if ((bmp_info.compression == BI_RLE8) && (bmp_info.bits_per_pixel != 8))
      ThrowReaderException(CorruptImageError,"UnsupportedBitsPerPixel");
    if ((bmp_info.compression == BI_RLE4) && (bmp_info.bits_per_pixel != 4))
      ThrowReaderException(CorruptImageError,"UnsupportedBitsPerPixel");
    if ((bmp_info.compression == BI_BITFIELDS) &&
        (bmp_info.bits_per_pixel < 16))
      ThrowReaderException(CorruptImageError,"UnsupportedBitsPerPixel");
    switch (bmp_info.compression)
    {
      case BI_RGB:
        image->compression=NoCompression;
        break;
      case BI_RLE8:
      case BI_RLE4:
        image->compression=RLECompression;
        break;
      case BI_BITFIELDS:
        break;
      case BI_ALPHABITFIELDS:
        break;
      case BI_JPEG:
        ThrowReaderException(CoderError,"JPEGCompressNotSupported");
      case BI_PNG:
        ThrowReaderException(CoderError,"PNGCompressNotSupported");
      default:
        ThrowReaderException(CorruptImageError,"UnrecognizedImageCompression");
    }
    image->columns=(size_t) MagickAbsoluteValue(bmp_info.width);
    image->rows=(size_t) MagickAbsoluteValue(bmp_info.height);
    image->depth=bmp_info.bits_per_pixel <= 8 ? bmp_info.bits_per_pixel : 8;
    image->alpha_trait=((bmp_info.alpha_mask != 0) &&
      ((bmp_info