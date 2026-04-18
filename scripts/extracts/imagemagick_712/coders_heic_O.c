// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 15/17



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
  scene=0;
  heif_context=heif_context_alloc();
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,7,0)
  encode_avif=(LocaleCompare(image_info->magick,"AVIF") == 0) ? MagickTrue :
    MagickFalse;
#endif
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,20,0)
  if ((encode_avif != MagickFalse) && (image_info->adjoin != MagickFalse) &&
      (GetNextImageInList(image) != (Image *) NULL))
    {
      Image
        *coalesce_image,
        *frame;

      (void) CloseBlob(image);
      heif_context_free(heif_context);

      coalesce_image=(Image *) NULL;
      frame=image;
      while (frame != (Image *) NULL)
      {
        if ((frame->rows != image->rows) || (frame->columns != image->columns) ||
            (frame->page.x != image->page.x) || (frame->page.y != image->page.y) ||
            (frame->dispose != UndefinedDispose) ||
            ((frame->alpha_trait & BlendPixelTrait) != 0))
          {
            coalesce_image=CoalesceImages(image,exception);
            break;
          }
        frame=GetNextImageInList(frame);
      }
      if (coalesce_image != (Image *) NULL)
        {
          status=WriteHEICSequenceImage(image_info,coalesce_image,exception);
          (void) DestroyImageList(coalesce_image);
          return(status);
        }
      return(WriteHEICSequenceImage(image_info,image,exception));
    }
#endif
  do
  {
    const char
      *option;

    const StringInfo
      *profile;

    enum heif_chroma
      chroma;

    enum heif_colorspace
      colorspace = heif_colorspace_YCbCr;

    MagickBooleanType
      lossless = image_info->quality >= 100 ? MagickTrue : MagickFalse;

    struct heif_encoding_options
      *options;

    /*
      Get encoder for the specified format.
    */
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,7,0)
    if (encode_avif != MagickFalse)
      error=heif_context_get_encoder_for_format(heif_context,
        heif_compression_AV1,&heif_encoder);
    else
#endif
      error=heif_context_get_encoder_for_format(heif_context,
        heif_compression_HEVC,&heif_encoder);
    if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
      break;
    status=IsHEIFSuccess(image,&error,exception);
    if (status == MagickFalse)
      break;
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
      Initialize HEIF encoder context.
    */
    error=heif_image_create((int) image->columns,(int) image->rows,colorspace,
      chroma,&heif_image);
    status=IsHEIFSuccess(image,&error,exception);
    if (status == MagickFalse)
      break;
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,17,0)
    option=GetImageOption(image_info,"heic:cicp");
    if (option != (char *) NULL)
      {
        GeometryInfo
          cicp;

        struct heif_color_profile_nclx
          *nclx_profile;