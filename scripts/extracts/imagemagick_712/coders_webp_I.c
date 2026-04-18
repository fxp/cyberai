// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/webp.c
// Segment 9/11



  icc_profile=GetImageProfile(image,"ICC");
  exif_profile=GetImageProfile(image,"EXIF");
  xmp_profile=GetImageProfile(image,"XMP");
  if ((icc_profile == (StringInfo *) NULL) &&
      (exif_profile == (StringInfo *) NULL) &&
      (xmp_profile == (StringInfo *) NULL) &&
      (image->iterations == 0))
    return(MagickTrue);
  mux=WebPMuxCreate(webp_data, 1);
  WebPDataClear(webp_data);
  if (mux == NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),ResourceLimitError,
      "UnableToEncodeImageFile","`%s'",image->filename);
  /*
    Clean up returned data.
  */
  memset(webp_data, 0, sizeof(*webp_data));
  mux_error=WEBP_MUX_OK;
  if (image->iterations > 0)
    {
      /*
        If there is only 1 frame, webp_data will be created by
        WriteSingleWEBPImage and WebPMuxGetAnimationParams returns
        WEBP_MUX_NOT_FOUND.
      */
      mux_error=WebPMuxGetAnimationParams(mux, &new_params);
      if (mux_error == WEBP_MUX_NOT_FOUND)
        mux_error=WEBP_MUX_OK;
      else
        if (mux_error == WEBP_MUX_OK)
          {
            new_params.loop_count=MagickMin((int) image->iterations,65535);
            mux_error=WebPMuxSetAnimationParams(mux, &new_params);
          }
    }
  if ((icc_profile != (StringInfo *) NULL) && (mux_error == WEBP_MUX_OK))
    {
      chunk.bytes=GetStringInfoDatum(icc_profile);
      chunk.size=GetStringInfoLength(icc_profile);
      mux_error=WebPMuxSetChunk(mux,"ICCP",&chunk,0);
    }
  if ((exif_profile != (StringInfo *) NULL) && (mux_error == WEBP_MUX_OK))
    {
      chunk.bytes=GetStringInfoDatum(exif_profile);
      chunk.size=GetStringInfoLength(exif_profile);
      if ((chunk.size >= 6) &&
          (chunk.bytes[0] == 'E') && (chunk.bytes[1] == 'x') &&
          (chunk.bytes[2] == 'i') && (chunk.bytes[3] == 'f') &&
          (chunk.bytes[4] == '\0') && (chunk.bytes[5] == '\0'))
        {
          chunk.bytes=GetStringInfoDatum(exif_profile)+6;
          chunk.size-=6;
        }
      mux_error=WebPMuxSetChunk(mux,"EXIF",&chunk,0);
    }
  if ((xmp_profile != (StringInfo *) NULL) && (mux_error == WEBP_MUX_OK))
    {
      chunk.bytes=GetStringInfoDatum(xmp_profile);
      chunk.size=GetStringInfoLength(xmp_profile);
      mux_error=WebPMuxSetChunk(mux,"XMP ",&chunk,0);
    }
  if (mux_error == WEBP_MUX_OK)
    mux_error=WebPMuxAssemble(mux,webp_data);
  WebPMuxDelete(mux);
  if (mux_error != WEBP_MUX_OK)
    (void) ThrowMagickException(exception,GetMagickModule(),ResourceLimitError,
      "UnableToEncodeImageFile","`%s'",image->filename);
  return(MagickTrue);
}
#endif

static inline void SetBooleanOption(const ImageInfo *image_info,
  const char *option,int *setting)
{
  const char
    *value;

  value=GetImageOption(image_info,option);
  if (value != (char *) NULL)
    *setting=(int) ParseCommandOption(MagickBooleanOptions,MagickFalse,value);
}

static inline void SetIntegerOption(const ImageInfo *image_info,
  const char *option,int *setting)
{
  const char
    *value;

  value=GetImageOption(image_info,option);
  if (value != (const char *) NULL)
    *setting=StringToInteger(value);
}

static MagickBooleanType WriteWEBPImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo * exception)
{
  const char
    *value;

  MagickBooleanType
    status;

  WebPConfig
    configure;

  WebPMemoryWriter
    writer;