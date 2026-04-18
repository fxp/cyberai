// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jxl.c
// Segment 3/11



static inline void JXLSetFormat(Image *image,JxlPixelFormat *pixel_format,
  ExceptionInfo *exception)
{
  const char
    *property;

  pixel_format->num_channels=((image->alpha_trait & BlendPixelTrait) != 0) ?
    4U : 3U;
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    pixel_format->num_channels=((image->alpha_trait & BlendPixelTrait) != 0) ?
      2U : 1U;
  pixel_format->data_type=(image->depth > 16) ? JXL_TYPE_FLOAT :
    (image->depth > 8) ? JXL_TYPE_UINT16 : JXL_TYPE_UINT8;
  property=GetImageProperty(image,"quantum:format",exception);
  if (property != (char *) NULL)
    {
      QuantumFormatType format = (QuantumFormatType) ParseCommandOption(
        MagickQuantumFormatOptions,MagickFalse,property);
      if ((format == FloatingPointQuantumFormat) &&
          (pixel_format->data_type == JXL_TYPE_UINT16))
        {
          pixel_format->data_type=JXL_TYPE_FLOAT16;
          (void) SetImageProperty(image,"quantum:format","floating-point",
            exception);
        }
    }
}

static inline void JXLInitImage(Image *image,JxlBasicInfo *basic_info)
{
  image->columns=basic_info->xsize;
  image->rows=basic_info->ysize;
  image->depth=basic_info->bits_per_sample;
  if (basic_info->alpha_bits != 0)
    image->alpha_trait=BlendPixelTrait;
  image->orientation=JXLOrientationToOrientation(basic_info->orientation);
  if (basic_info->have_animation == JXL_TRUE)
    {
      if ((basic_info->animation.tps_numerator > 0) &&
          (basic_info->animation.tps_denominator > 0))
      image->ticks_per_second=basic_info->animation.tps_numerator /
        basic_info->animation.tps_denominator;
      image->iterations=basic_info->animation.num_loops;
    }
}

static inline MagickBooleanType JXLPatchExifProfile(StringInfo *exif_profile)
{
  size_t
    exif_length;

  StringInfo
    *snippet;

  unsigned char
    *exif_datum;

  unsigned int
    offset=0;

  if (GetStringInfoLength(exif_profile) < 4)
    return(MagickFalse);

  /*
    Extract and cache Exif profile.
  */
  snippet=SplitStringInfo(exif_profile,4);
  offset|=(unsigned int) (*(GetStringInfoDatum(snippet)+0)) << 24;
  offset|=(unsigned int) (*(GetStringInfoDatum(snippet)+1)) << 16;
  offset|=(unsigned int) (*(GetStringInfoDatum(snippet)+2)) << 8;
  offset|=(unsigned int) (*(GetStringInfoDatum(snippet)+3)) << 0;
  snippet=DestroyStringInfo(snippet);
  /*
    Strip any EOI marker if payload starts with a JPEG marker.
  */
  exif_length=GetStringInfoLength(exif_profile);
  exif_datum=GetStringInfoDatum(exif_profile);
  if ((exif_length > 2) && 
      ((memcmp(exif_datum,"\xff\xd8",2) == 0) ||
        (memcmp(exif_datum,"\xff\xe1",2) == 0)) &&
      (memcmp(exif_datum+exif_length-2,"\xff\xd9",2) == 0))
    SetStringInfoLength(exif_profile,exif_length-2);
  /*
    Skip to actual Exif payload.
  */
  if (offset < GetStringInfoLength(exif_profile))
    (void) DestroyStringInfo(SplitStringInfo(exif_profile,offset));
  return(MagickTrue);
}

static inline void JXLAddProfilesToImage(Image *image,
  StringInfo **exif_profile,StringInfo **xmp_profile,ExceptionInfo *exception)
{
  if (*exif_profile != (StringInfo *) NULL)
    {
      if (JXLPatchExifProfile(*exif_profile) != MagickFalse)
        {
          (void) SetImageProfilePrivate(image,*exif_profile,exception);
          *exif_profile=(StringInfo *) NULL;
        }
      else
        *exif_profile=DestroyStringInfo(*exif_profile);
    }
  if (*xmp_profile != (StringInfo *) NULL)
    {
      (void) SetImageProfilePrivate(image,*xmp_profile,exception);
      *xmp_profile=(StringInfo *) NULL;
    }
}

static Image *ReadJXLImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  Image
    *image;

  JxlBasicInfo
    basic_info;

  JxlDecoder
    *jxl_info;

  JxlDecoderStatus
    events_wanted,
    jxl_status;

  JxlMemoryManager
    memory_manager;

  JxlPixelFormat
    pixel_format;

  MagickBooleanType
    status;

  MemoryManagerInfo
    memory_manager_info;

  size_t
    extent = 0,
    image_count = 0,
    input_size;

  StringInfo
    *exif_profile = (StringInfo *) NULL,
    *xmp_profile = (StringInfo *) NULL;

  unsigned char
    *pixels,
    *output_buffer = (unsigned char *) NULL;

  void
    *runner = NULL;