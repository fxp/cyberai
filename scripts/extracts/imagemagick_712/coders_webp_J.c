// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/webp.c
// Segment 10/11



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
  if ((image->columns > 16383UL) || (image->rows > 16383UL))
    ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");
  if (WebPConfigInit(&configure) == 0)
    ThrowWriterException(ResourceLimitError,"UnableToEncodeImageFile");
  if (image->quality != UndefinedCompressionQuality)
    {
      configure.quality=(float) image->quality;
#if WEBP_ENCODER_ABI_VERSION >= 0x020e
      configure.near_lossless=(int) image->quality;
#endif
    }
  if (image->quality >= 100)
    configure.lossless=1;
  SetBooleanOption(image_info,"webp:lossless",&configure.lossless);
  value=GetImageOption(image_info,"webp:image-hint");
  if (value != (char *) NULL)
    {
      if (LocaleCompare(value,"default") == 0)
        configure.image_hint=WEBP_HINT_DEFAULT;
      if (LocaleCompare(value,"photo") == 0)
        configure.image_hint=WEBP_HINT_PHOTO;
      if (LocaleCompare(value,"picture") == 0)
        configure.image_hint=WEBP_HINT_PICTURE;
#if WEBP_ENCODER_ABI_VERSION >= 0x0200
      if (LocaleCompare(value,"graph") == 0)
        configure.image_hint=WEBP_HINT_GRAPH;
#endif
    }
  SetBooleanOption(image_info,"webp:auto-filter",&configure.autofilter);
  value=GetImageOption(image_info,"webp:target-psnr");
  if (value != (char *) NULL)
    configure.target_PSNR=(float) StringToDouble(value,(char **) NULL);
  SetIntegerOption(image_info,"webp:alpha-compression",
    &configure.alpha_compression);
  SetIntegerOption(image_info,"webp:alpha-filtering",
    &configure.alpha_filtering);
  SetIntegerOption(image_info,"webp:alpha-quality",&configure.alpha_quality);
  SetIntegerOption(image_info,"webp:filter-strength",
    &configure.filter_strength);
  SetIntegerOption(image_info,"webp:filter-sharpness",
    &configure.filter_sharpness);
  SetIntegerOption(image_info,"webp:filter-type",&configure.filter_type);
  SetIntegerOption(image_info,"webp:method",&configure.method);
  SetIntegerOption(image_info,"webp:partitions",&configure.partitions);
  SetIntegerOption(image_info,"webp:partition-limit",
    &configure.partition_limit);
  SetIntegerOption(image_info,"webp:pass",&configure.pass);
  SetIntegerOption(image_info,"webp:preprocessing",&configure.preprocessing);
  SetIntegerOption(image_info,"webp:segments",&configure.segments);
  SetIntegerOption(image_info,"webp:show-compressed",
    &configure.show_compressed);
  SetIntegerOption(image_info,"webp:sns-strength",&configure.sns_strength);
  SetIntegerOption(image_info,"webp:target-size",&configure.target_size);
#if WEBP_ENCODER_ABI_VERSION >= 0x0201
  SetBooleanOption(image_info,"webp:emulate-jpeg-size",
    &configure.emulate_jpeg_size);
  SetBooleanOption(image_info,"webp:low-memory",&configure.low_memory);
  SetIntegerOption(image_info,"webp:thread-level",&configure.thread_level);
#endif
#if WEBP_ENCODER_ABI_VERSION >= 0x0209
  SetBooleanOption(image_info,"webp:exact",&configure.exact);
#endif
#if WEBP_ENCODER_ABI_VERSION >= 0x020e
  SetBooleanOption(image_info,"webp:use-sharp-yuv",&configure.use_sharp_yuv);
#endif
  if (((configure.target_size > 0) || (configure.target_PSNR > 0)) &&
      (configure.pass == 1))
    configure.pass=6;
  if (WebPValidateConfig(&configure) == 0)
    ThrowWriterException(ResourceLimitError,"UnableToEncodeImageFile");
#if defined(MAGICKCORE_WEBPMUX_DELEGATE)
  {
    Image
      *next;

    WebPData
      webp_data;

    memset(&webp_data,0,sizeof(webp_data));
    next=GetNextImageInList(image);
    if ((next != (Image *) NULL) && (image_info->adjoin != MagickFalse))
      {
        Image
          *coalesce_image=(Image *) NULL;