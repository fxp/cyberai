// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 7/17



    if (AcquireMagickResource(ListLengthResource,scene+1) == MagickFalse)
      {
        status=MagickFalse;
        break;
      }
    heif_image=(struct heif_image *) NULL;
    error=heif_track_decode_next_image(track,&heif_image,heif_colorspace_RGB,
      chroma,decode_options);
    if (error.code == heif_error_End_of_sequence)
      break;
    if (error.code != 0)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          CorruptImageError,error.message,"(%d.%d) `%s'",error.code,
          error.subcode,image->filename);
        status=MagickFalse;
        break;
      }
    /*
      Allocate next image for frames beyond the first.
    */
    if (scene > 0)
      {
        AcquireNextImage(image_info,image,exception);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            heif_image_release(heif_image);
            status=MagickFalse;
            break;
          }
        image=SyncNextImageInList(image);
      }
    image->scene=scene;
    /*
      Set frame timing: convert track timescale ticks to ticks_per_second.
    */
    duration=heif_image_get_duration(heif_image);
    image->ticks_per_second=(ssize_t) timescale;
    image->delay=(size_t) duration;
    image->iterations=0;
    /*
      Set image dimensions from the decoded frame.
    */
    channel=heif_channel_interleaved;
    image->columns=(size_t) heif_image_get_width(heif_image,channel);
    image->rows=(size_t) heif_image_get_height(heif_image,channel);
    bits_per_pixel=heif_image_get_bits_per_pixel_range(heif_image,channel);
    if (bits_per_pixel > 0)
      image->depth=(size_t) bits_per_pixel;
    if (has_alpha != 0)
      {
        image->alpha_trait=BlendPixelTrait;
        image->dispose=BackgroundDispose;
      }
    if (image_info->ping != MagickFalse)
      {
        heif_image_release(heif_image);
        scene++;
        continue;
      }
    if (HEICSkipImage(image_info,image) != MagickFalse)
      {
        heif_image_release(heif_image);
        scene++;
        if (image_info->number_scenes != 0)
          if (image->scene >= (image_info->scene+image_info->number_scenes-1))
            break;
        continue;
      }
    status=SetImageExtent(image,image->columns,image->rows,exception);
    if (status == MagickFalse)
      {
        heif_image_release(heif_image);
        break;
      }
    pixels=heif_image_get_plane_readonly(heif_image,channel,&stride);
    if (pixels == (const uint8_t *) NULL)
      {
        heif_image_release(heif_image);
        status=MagickFalse;
        break;
      }
    shift=(int) (16-image->depth);
    if (image->depth <= 8)
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        Quantum
          *q;

        ssize_t
          x;

        q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
        if (q == (Quantum *) NULL)
          break;
        p=pixels+(y*stride);
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          SetPixelRed(image,ScaleCharToQuantum((unsigned char) *(p++)),q);
          SetPixelGreen(image,ScaleCharToQuantum((unsigned char) *(p++)),q);
          SetPixelBlue(image,ScaleCharToQuantum((unsigned char) *(p++)),q);
          if (image->alpha_trait != UndefinedPixelTrait)
            SetPixelAlpha(image,ScaleCharToQuantum((unsigned char) *(p++)),q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        if (SyncAuthenticPixels(image,exception) == MagickFalse)
          break;
      }
    else
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        Quantum
          *q;

        ssize_t
          x;

        q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
        if (q == (Quantum *) NULL)
          break;
        p=pixels+(y*stride);
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          unsigned short pixel = (((unsigned short) *(p+1) << 8) |
            (*(p+0))) << shift; p+=(ptrdiff_t) 2;
          SetPixelRed(image,ScaleShortToQuantum(pixel),q);
          pixel=(((unsigned short) *(p+1) << 8) | (*(p+0))) << shift; p+=(ptrdiff_t) 2;
          SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
          pixel=(((unsigned short) *(p+1) << 8) | (*(p+0))) << shift; p+=(ptrdiff_t) 2;
          SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
          if (image->alpha_trait != UndefinedPixelTrait)
            {
              pixel=(((unsigned short) *(p+1) << 8) | (*(p+0))) << shift; p+=(ptrdiff_t) 2;
              SetPixelAlpha(image,ScaleShortToQuantum(pixel),q);
            }
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        if (SyncAuthenticPixels(image,exception) == MagickFalse)
          break;
      }
    heif_image_release(heif_image);
    scene++;
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
  }
  heif_decoding_options_free(decode_options);
  heif_track_release(track);
  return(status);
}
#endif