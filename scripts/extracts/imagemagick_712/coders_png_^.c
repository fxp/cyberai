// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 30/94



    if (memcmp(type,mng_sRGB,4) == 0)
      {
        if (length == 1)
          {
            image->rendering_intent=
              Magick_RenderingIntent_from_PNG_RenderingIntent(p[0]);
            image->gamma=0.45455f;
            image->chromaticity.red_primary.x=0.6400f;
            image->chromaticity.red_primary.y=0.3300f;
            image->chromaticity.green_primary.x=0.3000f;
            image->chromaticity.green_primary.y=0.6000f;
            image->chromaticity.blue_primary.x=0.1500f;
            image->chromaticity.blue_primary.y=0.0600f;
            image->chromaticity.white_point.x=0.3127f;
            image->chromaticity.white_point.y=0.3290f;
          }

        chunk=(unsigned char *) RelinquishMagickMemory(chunk);
        continue;
      }

    if (memcmp(type,mng_oFFs,4) == 0)
      {
        if (length > 8)
          {
            image->page.x=(ssize_t) mng_get_long(p);
            image->page.y=(ssize_t) mng_get_long(&p[4]);

            if ((int) p[8] != 0)
              {
                image->page.x/=10000;
                image->page.y/=10000;
              }
          }

        chunk=(unsigned char *) RelinquishMagickMemory(chunk);

        continue;
      }

    if (memcmp(type,mng_pHYs,4) == 0)
      {
        if (length > 8)
          {
            image->resolution.x=(double) mng_get_long(p);
            image->resolution.y=(double) mng_get_long(&p[4]);
            if ((int) p[8] == PNG_RESOLUTION_METER)
              {
                image->units=PixelsPerCentimeterResolution;
                image->resolution.x=image->resolution.x/100.0;
                image->resolution.y=image->resolution.y/100.0;
              }
          }

        chunk=(unsigned char *) RelinquishMagickMemory(chunk);
        continue;
      }

#if 0
    if (memcmp(type,mng_iCCP,4) == 0)
      {
        /* To do: */
        chunk=(unsigned char *) RelinquishMagickMemory(chunk);

        continue;
      }
#endif

    chunk=(unsigned char *) RelinquishMagickMemory(chunk);

    if (memcmp(type,mng_IEND,4))
      continue;

    break;
  }


  /* IEND found */

  /*
    Finish up reading image data:

       o read main image from color_blob.

       o close color_blob.

       o if (color_type has alpha)
            if alpha_encoding is PNG
               read secondary image from alpha_blob via ReadPNG
            if alpha_encoding is JPEG
               read secondary image from alpha_blob via ReadJPEG

       o close alpha_blob.

       o copy intensity of secondary image into
         alpha samples of main image.

       o destroy the secondary image.
  */

  if (color_image_info == (ImageInfo *) NULL)
    {
      assert(color_image == (Image *) NULL);
      assert(alpha_image == (Image *) NULL);
      if (color_image != (Image *) NULL)
        color_image=DestroyImageList(color_image);
      return(DestroyImageList(image));
    }

  if (color_image == (Image *) NULL)
    {
      assert(alpha_image == (Image *) NULL);
      ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
    }

  (void) SeekBlob(color_image,0,SEEK_SET);

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "    Reading jng_image from color_blob.");

  assert(color_image_info != (ImageInfo *) NULL);
  (void) FormatLocaleString(color_image_info->filename,MagickPathExtent,
    "jpeg:%s",color_image->filename);

  color_image_info->ping=MagickFalse;   /* To do: avoid this */
  CloseBlob(color_image);
  jng_image=ReadImage(color_image_info,exception);

  (void) RelinquishUniqueFileResource(color_image->filename);
  color_image=DestroyImageList(color_image);
  color_image_info=DestroyImageInfo(color_image_info);

  if (jng_image == (Image *) NULL)
  {
    DestroyJNG(NULL,NULL,NULL,&alpha_image,&alpha_image_info);
    return(DestroyImageList(image));
  }


  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "    Copying jng_image pixels to main image.");

  image->rows=jng_height;
  image->columns=jng_width;