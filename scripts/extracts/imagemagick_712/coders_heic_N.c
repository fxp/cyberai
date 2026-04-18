// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 14/17



    /*
      Determine colorspace and chroma for this frame.
    */
    colorspace=heif_colorspace_YCbCr;
    chroma=lossless != MagickFalse ? heif_chroma_444 : heif_chroma_420;
    if ((image->alpha_trait & BlendPixelTrait) != 0)
      {
        if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
          status=TransformImageColorspace(image,sRGBColorspace,exception);
        colorspace=heif_colorspace_RGB;
        chroma=heif_chroma_interleaved_RGBA;
        if (image->depth > 8)
          chroma=heif_chroma_interleaved_RRGGBBAA_LE;
      }
    else
      if (IssRGBCompatibleColorspace(image->colorspace) != MagickFalse)
        {
          colorspace=heif_colorspace_RGB;
          chroma=heif_chroma_interleaved_RGB;
          if (image->depth > 8)
            chroma=heif_chroma_interleaved_RRGGBB_LE;
          if (GetPixelChannels(image) == 1)
            {
              colorspace=heif_colorspace_monochrome;
              chroma=heif_chroma_monochrome;
            }
        }
      else
        if (image->colorspace != YCbCrColorspace)
          status=TransformImageColorspace(image,YCbCrColorspace,exception);
    if (status == MagickFalse)
      break;
    /*
      Create heif_image for this frame.
    */
    error=heif_image_create((int) image->columns,(int) image->rows,colorspace,
      chroma,&heif_image);
    status=IsHEIFSuccess(image,&error,exception);
    if (status == MagickFalse)
      break;
    profile=GetImageProfile(image,"icc");
    if (profile != (StringInfo *) NULL)
      (void) heif_image_set_raw_color_profile(heif_image,"prof",
        GetStringInfoDatum(profile),GetStringInfoLength(profile));
    /*
      Fill heif_image pixels from ImageMagick image.
    */
    if (colorspace == heif_colorspace_YCbCr)
      status=WriteHEICImageYCbCr(image,heif_image,exception);
    else
      if (image->depth > 8)
        status=WriteHEICImageRRGGBBAA(image,heif_image,exception);
      else
        status=WriteHEICImageRGBA(image,heif_image,exception);
    if (status == MagickFalse)
      {
        heif_image_release(heif_image);
        heif_image=(struct heif_image *) NULL;
        break;
      }
    /*
      Set frame duration and encode into the track.
    */
    if (image->delay > (size_t) UINT32_MAX)
      duration=UINT32_MAX;
    else
      duration=(uint32_t) image->delay;
    if (duration == 0)
      duration=timescale/10;
    heif_image_set_duration(heif_image,duration);
    error=heif_track_encode_sequence_image(track,heif_image,heif_encoder,
      seq_options);
    heif_image_release(heif_image);
    heif_image=(struct heif_image *) NULL;
    status=IsHEIFSuccess(image,&error,exception);
    if (status == MagickFalse)
      break;
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    status=SetImageProgress(image,SaveImagesTag,scene,
      GetImageListLength(image));
    if (status == MagickFalse)
      break;
    scene++;
  } while (image_info->adjoin != MagickFalse);
  /*
    Finalize the sequence and write to output.
  */
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,21,0)
  if (status != MagickFalse)
    {
      struct heif_writer
        writer;

      error=heif_track_encode_end_of_sequence(track,heif_encoder);
      if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
        status=MagickFalse;
      if (status != MagickFalse)
        {
          writer.writer_api_version=1;
          writer.write=heif_write_func;
          error=heif_context_write(heif_context,&writer,image);
          status=IsHEIFSuccess(image,&error,exception);
        }
    }
#endif
  if (seq_options != (struct heif_sequence_encoding_options *) NULL)
    heif_sequence_encoding_options_release(seq_options);
  if (track != (heif_track *) NULL)
    heif_track_release(track);
  heif_encoder_release(heif_encoder);
  heif_context_free(heif_context);
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  return(status);
}
#endif

static MagickBooleanType WriteHEICImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  MagickBooleanType
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,7,0)
    encode_avif,
#endif
    status;

  MagickOffsetType
    scene;

  struct heif_context
    *heif_context;

  struct heif_encoder
    *heif_encoder = (struct heif_encoder*) NULL;

  struct heif_error
    error;

  struct heif_image
    *heif_image = (struct heif_image*) NULL;