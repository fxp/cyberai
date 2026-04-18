// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jxl.c
// Segment 8/11



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r J X L I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterJXLImage() removes format registrations made by the
%  JXL module from the list of supported formats.
%
%  The format of the UnregisterJXLImage method is:
%
%      UnregisterJXLImage(void)
%
*/
ModuleExport void UnregisterJXLImage(void)
{
  (void) UnregisterMagickInfo("JXL");
}

#if defined(MAGICKCORE_JXL_DELEGATE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  W r i t e J X L I m a g e                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteJXLImage() writes a JXL image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the WriteJXLImage method is:
%
%      MagickBooleanType WriteJXLImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image:  The image.
%
*/

static JxlEncoderStatus JXLWriteMetadata(const Image *image,
  JxlEncoder *jxl_info, const StringInfo *icc_profile)
{
  JxlColorEncoding
    color_encoding;

  JxlEncoderStatus
    jxl_status;

  if (icc_profile != (StringInfo *) NULL)
    {
      jxl_status=JxlEncoderSetICCProfile(jxl_info,(const uint8_t *)
        GetStringInfoDatum(icc_profile),GetStringInfoLength(icc_profile));
      return(jxl_status);
    }
  (void) memset(&color_encoding,0,sizeof(color_encoding));
  color_encoding.color_space=JXL_COLOR_SPACE_RGB;
  if (IsRGBColorspace(image->colorspace) == MagickFalse)
    JxlColorEncodingSetToSRGB(&color_encoding,
      IsGrayColorspace(image->colorspace) != MagickFalse);
  else
    JxlColorEncodingSetToLinearSRGB(&color_encoding,
      IsGrayColorspace(image->colorspace) != MagickFalse);
  jxl_status=JxlEncoderSetColorEncoding(jxl_info,&color_encoding);
  return(jxl_status);
}

static inline float JXLGetDistance(float quality)
{
  return quality >= 100.0f ? 0.0f
         : quality >= 30
             ? 0.1f + (100 - quality) * 0.09f
             : 53.0f / 3000.0f * quality * quality - 23.0f / 20.0f * quality + 25.0f;
}

static inline MagickBooleanType JXLSameFrameType(const Image *image,
  const Image *frame)
{
  if (image->depth != frame->depth)
    return(MagickFalse);
  if (image->alpha_trait != frame->alpha_trait)
    return(MagickFalse);
  if (image->colorspace != frame->colorspace)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType WriteJXLImage(const ImageInfo *image_info,Image *image,
  ExceptionInfo *exception)
{
  const char
    *option;

  const StringInfo
    *icc_profile = (StringInfo *) NULL,
    *exif_profile = (StringInfo *) NULL,
    *xmp_profile = (StringInfo *) NULL;

  JxlBasicInfo
    basic_info;

  JxlEncoder
    *jxl_info;

  JxlEncoderFrameSettings
    *frame_settings;

  JxlEncoderStatus
    jxl_status;

  JxlFrameHeader
    frame_header;

  JxlMemoryManager
    memory_manager;

  JxlPixelFormat
    pixel_format;

  MagickBooleanType
    status;

  MemoryInfo
    *pixel_info = (MemoryInfo *) NULL;

  MemoryManagerInfo
    memory_manager_info;

  size_t
    channels_size,
    sample_size;

  unsigned char
    *pixels;

  void
    *runner;