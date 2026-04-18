// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 31/94



  status=SetImageExtent(image,image->columns,image->rows,exception);
  if (status == MagickFalse)
    {
      DestroyJNG(NULL,&color_image,&color_image_info,&alpha_image,
        &alpha_image_info);
      jng_image=DestroyImageList(jng_image);
      return(DestroyImageList(image));
    }
  if ((image->columns != jng_image->columns) ||
      (image->rows != jng_image->rows))
    {
      DestroyJNG(NULL,&color_image,&color_image_info,&alpha_image,
        &alpha_image_info);
      jng_image=DestroyImageList(jng_image);
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    }
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    s=GetVirtualPixels(jng_image,0,y,image->columns,1,exception);
    q=GetAuthenticPixels(image,0,y,image->columns,1,exception);
    if ((s == (const Quantum *)  NULL) || (q == (Quantum *) NULL))
      break;
    for (x=(ssize_t) image->columns; x != 0; x--)
    {
      SetPixelRed(image,GetPixelRed(jng_image,s),q);
      SetPixelGreen(image,GetPixelGreen(jng_image,s),q);
      SetPixelBlue(image,GetPixelBlue(jng_image,s),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
      s+=(ptrdiff_t) GetPixelChannels(jng_image);
    }

    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }

  jng_image=DestroyImage(jng_image);

  if ((image_info->ping == MagickFalse) && (alpha_image != (Image *) NULL) &&
      (jng_color_type >= 12))
    {
      switch (jng_alpha_compression_method)
      {
        case 0:
        {
          png_byte
            data[5];

          (void) FormatLocaleString(alpha_image_info->filename,MagickPathExtent,
            "png:%s",alpha_image->filename);
          (void) WriteBlobMSBULong(alpha_image,0x00000000L);
          PNGType(data,mng_IEND);
          LogPNGChunk(logging,mng_IEND,0L);
          (void) WriteBlob(alpha_image,4,data);
          (void) WriteBlobMSBULong(alpha_image,crc32(0,data,4));
          break;
        }
        case 8:
        {
          (void) FormatLocaleString(alpha_image_info->filename,MagickPathExtent,
            "jpeg:%s",alpha_image->filename);
          break;
        }
        default:
        {
          (void) FormatLocaleString(alpha_image_info->filename,MagickPathExtent,
            "alpha:%s",alpha_image->filename);
          break;
        }
      }
      (void) CloseBlob(alpha_image);

      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    Reading alpha from alpha_blob.");
      jng_image=ReadImage(alpha_image_info,exception);

      if (jng_image != (Image *) NULL)
        {
          image->alpha_trait=BlendPixelTrait;
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            s=GetVirtualPixels(jng_image,0,y,image->columns,1,exception);
            q=GetAuthenticPixels(image,0,y,image->columns,1,exception);
            if ((s == (const Quantum *)  NULL) || (q == (Quantum *) NULL))
              break;

            for (x=(ssize_t) image->columns; x != 0; x--)
            {
              SetPixelAlpha(image,GetPixelRed(jng_image,s),q);
              q+=(ptrdiff_t) GetPixelChannels(image);
              s+=(ptrdiff_t) GetPixelChannels(jng_image);
            }
            if (SyncAuthenticPixels(image,exception) == MagickFalse)
              break;
          }
        }
      (void) RelinquishUniqueFileResource(alpha_image->filename);
      alpha_image=DestroyImageList(alpha_image);
      alpha_image_info=DestroyImageInfo(alpha_image_info);
      if (jng_image != (Image *) NULL)
        jng_image=DestroyImageList(jng_image);
    }

  if (alpha_image != (Image *) NULL)
    alpha_image=DestroyImageList(alpha_image);
  if (alpha_image_info != (ImageInfo *) NULL)
    alpha_image_info=DestroyImageInfo(alpha_image_info);

  /* Read the JNG image.  */

  if (mng_info->mng_type == 0)
    {
      mng_info->mng_width=jng_width;
      mng_info->mng_height=jng_height;
    }

  if (image->page.width == 0 && image->page.height == 0)
    {
      image->page.width=jng_width;
      image->page.height=jng_height;
    }

  if (image->page.x == 0 && image->page.y == 0)
    {
      image->page.x=mng_info->x_off[mng_info->object_id];
      image->page.y=mng_info->y_off[mng_info->object_id];
    }

  else
    {
      image->page.y=mng_info->y_off[mng_info->object_id];
    }

  mng_info->image_found++;
  status=SetImageProgress(image,LoadImagesTag,2*TellBlob(image),
    2*GetBlobSize(image));

  if (status == MagickFalse)
    return(DestroyImageList(image));

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  exit ReadOneJNGImage()");