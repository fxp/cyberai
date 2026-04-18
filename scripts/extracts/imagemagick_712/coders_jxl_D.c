// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jxl.c
// Segment 4/11



  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  image=AcquireImage(image_info, exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Initialize JXL delegate library.
  */
  (void) memset(&basic_info,0,sizeof(basic_info));
  (void) memset(&pixel_format,0,sizeof(pixel_format));
  JXLSetMemoryManager(&memory_manager,&memory_manager_info,image,exception);
  jxl_info=JxlDecoderCreate(&memory_manager);
  if (jxl_info == (JxlDecoder *) NULL)
    ThrowReaderException(CoderError,"MemoryAllocationFailed");
  (void) JxlDecoderSetKeepOrientation(jxl_info,JXL_TRUE);
  (void) JxlDecoderSetUnpremultiplyAlpha(jxl_info,JXL_TRUE);
  events_wanted=(JxlDecoderStatus) (JXL_DEC_BASIC_INFO | JXL_DEC_BOX |
    JXL_DEC_FRAME);
  if (image_info->ping == MagickFalse)
    {
      events_wanted=(JxlDecoderStatus) (events_wanted | JXL_DEC_FULL_IMAGE |
        JXL_DEC_COLOR_ENCODING);
      runner=JxlThreadParallelRunnerCreate(NULL,(size_t) GetMagickResourceLimit(
        ThreadResource));
      if (runner == (void *) NULL)
        {
          JxlDecoderDestroy(jxl_info);
          ThrowReaderException(CoderError,"MemoryAllocationFailed");
        }
      jxl_status=JxlDecoderSetParallelRunner(jxl_info,JxlThreadParallelRunner,
        runner);
      if (jxl_status != JXL_DEC_SUCCESS)
        {
          JxlThreadParallelRunnerDestroy(runner);
          JxlDecoderDestroy(jxl_info);
          ThrowReaderException(CoderError,"MemoryAllocationFailed");
        }
    }
  if (JxlDecoderSubscribeEvents(jxl_info,(int) events_wanted) != JXL_DEC_SUCCESS)
    {
      if (runner != NULL)
        JxlThreadParallelRunnerDestroy(runner);
      JxlDecoderDestroy(jxl_info);
      ThrowReaderException(CoderError,"UnableToReadImageData");
    }
  input_size=MagickMaxBufferExtent;
  pixels=(unsigned char *) AcquireQuantumMemory(input_size,sizeof(*pixels));
  if (pixels == (unsigned char *) NULL)
    {
      if (runner != NULL)
        JxlThreadParallelRunnerDestroy(runner);
      JxlDecoderDestroy(jxl_info);
      ThrowReaderException(CoderError,"MemoryAllocationFailed");
    }
  /*
    Decode JXL byte stream.
  */
  jxl_status=JXL_DEC_NEED_MORE_INPUT;
  while ((jxl_status != JXL_DEC_SUCCESS) && (jxl_status != JXL_DEC_ERROR))
  {
    jxl_status=JxlDecoderProcessInput(jxl_info);
    switch (jxl_status)
    {
      case JXL_DEC_NEED_MORE_INPUT:
      {
        size_t
          remaining;

        ssize_t
          count;

        remaining=JxlDecoderReleaseInput(jxl_info);
        if (remaining > 0)
          (void) memmove(pixels,pixels+input_size-remaining,remaining);
        count=ReadBlob(image,input_size-remaining,pixels+remaining);
        if (count <= 0)
          {
            JxlDecoderCloseInput(jxl_info);
            break;
          }
        jxl_status=JxlDecoderSetInput(jxl_info,(const uint8_t *) pixels,
          (size_t) count);
        if (jxl_status == JXL_DEC_SUCCESS)
          jxl_status=JXL_DEC_NEED_MORE_INPUT;
        break;
      }
      case JXL_DEC_BASIC_INFO:
      {
        jxl_status=JxlDecoderGetBasicInfo(jxl_info,&basic_info);
        if (jxl_status != JXL_DEC_SUCCESS)
          break;
        if ((basic_info.have_animation == JXL_TRUE) &&
            (basic_info.animation.have_timecodes == JXL_TRUE))
          {
            /*
              We currently don't support animations with time codes.
            */
            (void) ThrowMagickException(exception,GetMagickModule(),
              MissingDelegateError,"NoDecodeDelegateForThisImageFormat","`%s'",
              image->filename);
            break;
          }
        JXLInitImage(image,&basic_info);
        jxl_status=JXL_DEC_BASIC_INFO;
        break;
      }
      case JXL_DEC_COLOR_ENCODING:
      {
        JxlColorEncoding
          color_encoding;

        size_t
          profile_size;

        StringInfo
          *profile;