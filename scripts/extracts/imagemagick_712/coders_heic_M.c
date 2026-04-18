// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 13/17



  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(status);
  heif_context=heif_context_alloc();
  if (heif_context == (struct heif_context *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  /*
    Get encoder for AV1 (AVIF).
  */
  error=heif_context_get_encoder_for_format(heif_context,
    heif_compression_AV1,&heif_encoder);
  if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
    {
      heif_context_free(heif_context);
      return(MagickFalse);
    }
  lossless=image_info->quality >= 100 ? MagickTrue : MagickFalse;
  if (lossless != MagickFalse)
    (void) heif_encoder_set_lossless(heif_encoder,1);
  else if (image_info->quality != UndefinedCompressionQuality)
    (void) heif_encoder_set_lossy_quality(heif_encoder,(int)
      image_info->quality);
  option=GetImageOption(image_info,"heic:speed");
  if (option != (char *) NULL)
    (void) heif_encoder_set_parameter(heif_encoder,"speed",option);
  option=GetImageOption(image_info,"heic:chroma");
  if (option != (char *) NULL)
    (void) heif_encoder_set_parameter(heif_encoder,"chroma",option);
  /*
    Determine track timescale from the first frame.
  */
  if (image->ticks_per_second <= 0)
    timescale=100;
  else
    timescale=(uint32_t) image->ticks_per_second;
  heif_context_set_sequence_timescale(heif_context,timescale);
  /*
    Create the visual sequence track.
  */
  if ((image->columns > 65535) || (image->rows > 65535))
    {
      heif_encoder_release(heif_encoder);
      heif_context_free(heif_context);
      ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");
    }
  seq_options=heif_sequence_encoding_options_alloc();
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,21,0)
  if (seq_options != (struct heif_sequence_encoding_options *) NULL)
    seq_options->save_alpha_channel=1;
#endif
  track_options=heif_track_options_alloc();
  if (track_options != (struct heif_track_options *) NULL)
    heif_track_options_set_timescale(track_options,timescale);
  error=heif_context_add_visual_sequence_track(heif_context,
    (uint16_t) image->columns,(uint16_t) image->rows,
    heif_track_type_image_sequence,track_options,
    seq_options,&track);
  if (track_options != (struct heif_track_options *) NULL)
    heif_track_options_release(track_options);
  if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
    {
      if (seq_options != (struct heif_sequence_encoding_options *) NULL)
        heif_sequence_encoding_options_release(seq_options);
      heif_encoder_release(heif_encoder);
      heif_context_free(heif_context);
      return(MagickFalse);
    }
  {
    Image
      *frame;

    MagickBooleanType
      has_alpha;

    size_t
      depth;

    depth=image->depth;
    has_alpha=MagickFalse;
    for (frame=image; frame != (Image *) NULL; frame=GetNextImageInList(frame))
    {
      if ((frame->alpha_trait & BlendPixelTrait) != 0)
        has_alpha=MagickTrue;
      if (frame->depth > depth)
        depth=frame->depth;
    }
    for (frame=image; frame != (Image *) NULL; frame=GetNextImageInList(frame))
    {
      frame->depth=depth;
      if (has_alpha != MagickFalse)
        {
          if ((frame->alpha_trait & BlendPixelTrait) == 0)
            (void) SetImageAlphaChannel(frame,TransparentAlphaChannel,exception);
        }
    }
  }
  scene=0;
  status=MagickTrue;
  do
  {
    const StringInfo
      *profile;

    uint32_t
      duration;