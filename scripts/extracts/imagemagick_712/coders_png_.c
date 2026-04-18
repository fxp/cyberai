// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 84/94



      jpeg_image=SeparateImage(image,AlphaChannel,exception);
      if (jpeg_image == (Image *) NULL)
        ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
      (void) CopyMagickString(jpeg_image->magick,"JPEG",MagickPathExtent);
      jpeg_image->alpha_trait=UndefinedPixelTrait;
      jpeg_image->quality=jng_alpha_quality;
      jpeg_image_info->type=GrayscaleType;
      (void) SetImageType(jpeg_image,GrayscaleType,exception);
      (void) AcquireUniqueFilename(jpeg_image->filename);
      (void) FormatLocaleString(jpeg_image_info->filename,MagickPathExtent,
        "%s",jpeg_image->filename);
    }
  else
    {
      jng_alpha_compression_method=0;
      jng_color_type=10;
      jng_alpha_sample_depth=0;
    }

  /* To do: check bit depth of PNG alpha channel */

  /* Check if image is grayscale. */
  if ((image_info->type != TrueColorAlphaType) &&
      (image_info->type != TrueColorType))
    {
      if (IdentifyImageCoderGray(image,exception) != MagickFalse)
        jng_color_type-=2;
    }

  if (logging != MagickFalse)
    {
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    JNG Quality           = %d",(int) jng_quality);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    JNG Color Type        = %d",jng_color_type);
        if (transparent != 0)
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "    JNG Alpha Compression = %d",jng_alpha_compression_method);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "    JNG Alpha Depth       = %d",jng_alpha_sample_depth);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "    JNG Alpha Quality     = %d",(int) jng_alpha_quality);
          }
    }

  if (transparent != 0)
    {
      if (jng_alpha_compression_method==0)
        {
          const char
            *value;

          /* Encode alpha as a grayscale PNG blob */
          status=OpenBlob(jpeg_image_info,jpeg_image,WriteBinaryBlobMode,
            exception);
          if (status == MagickFalse)
            ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");

          if (logging != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Creating PNG blob.");

          (void) CopyMagickString(jpeg_image_info->magick,"PNG",
             MagickPathExtent);
          (void) CopyMagickString(jpeg_image->magick,"PNG",MagickPathExtent);
          jpeg_image_info->interlace=NoInterlace;

          /* Exclude all ancillary chunks */
          (void) SetImageArtifact(jpeg_image,"png:exclude-chunks","all");

          blob=(unsigned char *) ImageToBlob(jpeg_image_info,jpeg_image,
            &length,exception);

          if (blob == (unsigned char *) NULL)
            {
              jpeg_image=DestroyImage(jpeg_image);
              jpeg_image_info=DestroyImageInfo(jpeg_image_info);
              return(MagickFalse);
            }

          /* Retrieve sample depth used */
          value=GetImageProperty(jpeg_image,"png:bit-depth-written",exception);
          if (value != (char *) NULL)
            jng_alpha_sample_depth= (unsigned int) value[0];
        }
      else
        {
          /* Encode alpha as a grayscale JPEG blob */

          status=OpenBlob(jpeg_image_info,jpeg_image,WriteBinaryBlobMode,
            exception);
          if (status == MagickFalse)
            ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");

          (void) CopyMagickString(jpeg_image_info->magick,"JPEG",
            MagickPathExtent);
          (void) CopyMagickString(jpeg_image->magick,"JPEG",MagickPathExtent);
          jpeg_image_info->interlace=NoInterlace;
          if (logging != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Creating blob.");
          blob=(unsigned char *) ImageToBlob(jpeg_image_info,jpeg_image,
            &length,exception);
          if (blob == (unsigned char *) NULL)
            {
              jpeg_image=DestroyImage(jpeg_image);
              jpeg_image_info=DestroyImageInfo(jpeg_image_info);
              return(MagickFalse);
            }
          jng_alpha_sample_depth=8;

          if (logging != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Successfully read jpeg_image into a blob, length=%.20g.",
              (double) length);

        }
      /* Destroy JPEG image and image_info */
      jpeg_image=DestroyImage(jpeg_image);
      (void) RelinquishUniqueFileResource(jpeg_image_info->filename);
      jpeg_image_info=DestroyImageInfo(jpeg_image_info);
    }