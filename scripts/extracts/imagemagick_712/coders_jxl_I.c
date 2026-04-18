// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jxl.c
// Segment 9/11



  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(status);
  if ((IssRGBCompatibleColorspace(image->colorspace) == MagickFalse) &&
      (IsCMYKColorspace(image->colorspace) == MagickFalse))
    (void) TransformImageColorspace(image,sRGBColorspace,exception);
  if ((image_info->adjoin != MagickFalse) &&
      (GetNextImageInList(image) != (Image *) NULL))
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
              (void) SetImageAlphaChannel(frame,OpaqueAlphaChannel,exception);
          }
        if (frame->colorspace != image->colorspace)
          (void) TransformImageColorspace(frame,image->colorspace,exception);
      }
    }
  /*
    Initialize JXL delegate library.
  */
  (void) memset(&basic_info,0,sizeof(basic_info));
  (void) memset(&frame_header,0,sizeof(frame_header));
  (void) memset(&pixel_format,0,sizeof(pixel_format));
  JXLSetMemoryManager(&memory_manager,&memory_manager_info,image,exception);
  jxl_info=JxlEncoderCreate(&memory_manager);
  if (jxl_info == (JxlEncoder *) NULL)
    ThrowWriterException(CoderError,"MemoryAllocationFailed");
  runner=JxlThreadParallelRunnerCreate(NULL,(size_t) GetMagickResourceLimit(
    ThreadResource));
  if (runner == (void *) NULL)
    {
      JxlEncoderDestroy(jxl_info);
      ThrowWriterException(CoderError,"MemoryAllocationFailed");
    }
  jxl_status=JxlEncoderSetParallelRunner(jxl_info,JxlThreadParallelRunner,
    runner);
  if (jxl_status != JXL_ENC_SUCCESS)
    {
      JxlThreadParallelRunnerDestroy(runner);
      JxlEncoderDestroy(jxl_info);
      return(MagickFalse);
    }
  JXLSetFormat(image,&pixel_format,exception);
  JxlEncoderInitBasicInfo(&basic_info);
  basic_info.xsize=(uint32_t) image->columns;
  basic_info.ysize=(uint32_t) image->rows;
  basic_info.bits_per_sample=8;
  if (pixel_format.data_type == JXL_TYPE_UINT16)
    basic_info.bits_per_sample=16;
  else
    if (pixel_format.data_type == JXL_TYPE_FLOAT)
      {
        basic_info.bits_per_sample=32;
        basic_info.exponent_bits_per_sample=8;
      }
    else
      if (pixel_format.data_type == JXL_TYPE_FLOAT16)
        {
          basic_info.bits_per_sample=16;
          basic_info.exponent_bits_per_sample=8;
        }
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    basic_info.num_color_channels=1;
  if ((image->alpha_trait & BlendPixelTrait) != 0)
    {
      basic_info.alpha_bits=basic_info.bits_per_sample;
      basic_info.alpha_exponent_bits=basic_info.exponent_bits_per_sample;
      basic_info.num_extra_channels=1;
    }
  if (image_info->quality == 100)
    {
      basic_info.uses_original_profile=JXL_TRUE;
      icc_profile=GetImageProfile(image,"icc");
    }
  if ((image_info->adjoin != MagickFalse) &&
      (GetNextImageInList(image) != (Image *) NULL))
    {
      basic_info.have_animation=JXL_TRUE;
      basic_info.animation.num_loops=(uint32_t) image->iterations;
      basic_info.animation.tps_numerator=(uint32_t) image->ticks_per_second;
      basic_info.animation.tps_denominator=1;
      JxlEncoderInitFrameHeader(&frame_header);
    }
  jxl_status=JxlEncoderSetBasicInfo(jxl_info,&basic_info);
  if (jxl_status != JXL_ENC_SUCCESS)
    {
      JxlThreadParallelRunnerDestroy(runner);
      JxlEncoderDestroy(jxl_info);
      ThrowWriterException(CoderError,"UnableToWriteImageData");
    }
  frame_settings=JxlEncoderFrameSettingsCreate(jxl_info,
    (JxlEncoderFrameSettings *) NULL);
  if (frame_settings == (JxlEncoderFrameSettings *) NULL)
    {
      JxlThreadParallelRunnerDestroy(runner);
      JxlEncoderDestroy(jxl_info);
      ThrowWriterException(CoderError,"MemoryAllocationFailed");
    }
  if (image_info->quality == 100)
    {
      (void) JxlEncoderSetFrameDistance(frame_settings,0.f);
      (void) JxlEncoderSetFrameLossless(frame_settings,JXL_TRUE);
    }
  else if (image_info->quality != 0)
    (void) JxlEncoderSetFrameDistance(frame_settings,
      JXLGetDistance((