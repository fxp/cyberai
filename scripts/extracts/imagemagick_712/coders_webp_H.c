// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/webp.c
// Segment 8/11



  (void) WebPAnimEncoderOptionsInit(&enc_options);
  if (image_info->verbose != MagickFalse)
    enc_options.verbose=1;
  /*
    Appropriate default kmin, kmax values for lossy and lossless.
  */
  enc_options.kmin = configure->lossless ? 9 : 3;
  enc_options.kmax = configure->lossless ? 17 : 5;
  enc=WebPAnimEncoderNew((int) image->columns,(int) image->rows,&enc_options);
  if (enc == (WebPAnimEncoder*) NULL)
    return(MagickFalse);
  status=MagickTrue;
  effective_delta=0;
  frame_timestamp=0;
  memory_info_list=NewLinkedList(GetImageListLength(image));
  frame=image;
  while (frame != NULL)
  {
    status=(MagickBooleanType) WebPPictureInit(&picture);
    if (status == MagickFalse)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          ResourceLimitError,"UnableToEncodeImageFile","`%s'",image->filename);
        break;
      }
    status=WriteSingleWEBPPicture(image_info,frame,&picture,
      &memory_info,exception);
    if (status != MagickFalse)
      {
        status=(MagickBooleanType) WebPAnimEncoderAdd(enc,&picture,
          (int) frame_timestamp,configure);
        if (status == MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            CorruptImageError,WebPErrorCodeMessage(picture.error_code),"`%s'",
            image->filename);
      }
    if (memory_info != (MemoryInfo *) NULL)
      (void) AppendValueToLinkedList(memory_info_list,memory_info);
    WebPPictureFree(&picture);
    effective_delta=(size_t) (frame->delay*1000*MagickSafeReciprocal(
      (double) frame->ticks_per_second));
    if (effective_delta < 10)
      effective_delta=100; /* Consistent with gif2webp */
    frame_timestamp+=effective_delta;
    frame=GetNextImageInList(frame);
  }
  if (status != MagickFalse)
    {
      /*
        Add last null frame and assemble picture.
      */
      status=(MagickBooleanType) WebPAnimEncoderAdd(enc,(WebPPicture *) NULL,
        (int) frame_timestamp,configure);
      if (status != MagickFalse)
        status=(MagickBooleanType) WebPAnimEncoderAssemble(enc,webp_data);
      if (status == MagickFalse)
        (void) ThrowMagickException(exception,GetMagickModule(),CoderError,
          WebPAnimEncoderGetError(enc),"`%s'",image->filename);
    }
  memory_info_list=DestroyLinkedList(memory_info_list,WebPDestroyMemoryInfo);
  WebPAnimEncoderDelete(enc);
  return(status);
}

static MagickBooleanType WriteWEBPImageProfile(Image *image,
  WebPData *webp_data,ExceptionInfo *exception)
{
  const StringInfo
    *icc_profile,
    *exif_profile,
    *xmp_profile;
 
  WebPData
    chunk;
 
  WebPMux
    *mux;
 
  WebPMuxAnimParams
    new_params;
 
  WebPMuxError
    mux_error;