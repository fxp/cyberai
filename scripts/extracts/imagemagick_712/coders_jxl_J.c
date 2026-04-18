// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jxl.c
// Segment 10/11

float) image_info->quality));
  option=GetImageOption(image_info,"jxl:effort");
  if (option != (const char *) NULL)
    (void) JxlEncoderFrameSettingsSetOption(frame_settings,
      JXL_ENC_FRAME_SETTING_EFFORT,StringToInteger(option));
  option=GetImageOption(image_info,"jxl:decoding-speed");
  if (option != (const char *) NULL)
    (void) JxlEncoderFrameSettingsSetOption(frame_settings,
      JXL_ENC_FRAME_SETTING_DECODING_SPEED,StringToInteger(option));
  exif_profile=GetImageProfile(image,"exif");
  xmp_profile=GetImageProfile(image,"xmp");
  if ((exif_profile != (StringInfo *) NULL) ||
      (xmp_profile != (StringInfo *) NULL))
    {
      (void) JxlEncoderUseBoxes(jxl_info);
      if ((exif_profile != (StringInfo *) NULL) &&
          (GetStringInfoLength(exif_profile) > 6))
        {
          /*
            Add Exif profile.  Assumes "Exif\0\0" JPEG APP1 prefix.
          */
          StringInfo
            *profile;

          profile=BlobToStringInfo("\0\0\0\6",4);
          if (profile != (StringInfo *) NULL)
            {
              ConcatenateStringInfo(profile,exif_profile);
              (void) JxlEncoderAddBox(jxl_info,"Exif",
                GetStringInfoDatum(profile),GetStringInfoLength(profile),0);
              profile=DestroyStringInfo(profile);
            }
        }
      if (xmp_profile != (StringInfo *) NULL)
        {
          /*
            Add XMP profile.
          */
          (void) JxlEncoderAddBox(jxl_info,"xml ",GetStringInfoDatum(
            xmp_profile),GetStringInfoLength(xmp_profile),0);
        }
      (void) JxlEncoderCloseBoxes(jxl_info);
    }
  jxl_status=JXLWriteMetadata(image,jxl_info,icc_profile);
  if (jxl_status != JXL_ENC_SUCCESS)
    {
      JxlThreadParallelRunnerDestroy(runner);
      JxlEncoderDestroy(jxl_info);
      ThrowWriterException(CoderError,"UnableToWriteImageData");
    }
  /*
    Write image as a JXL stream.
  */
  sample_size=sizeof(char);
  if ((pixel_format.data_type == JXL_TYPE_FLOAT) ||
      (pixel_format.data_type == JXL_TYPE_FLOAT16))
    sample_size=sizeof(float);
  else
    if (pixel_format.data_type == JXL_TYPE_UINT16)
      sample_size=sizeof(short);
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    channels_size=(((image->alpha_trait & BlendPixelTrait) != 0) ? 2U : 1U)*
      sample_size;
  else
    channels_size=(((image->alpha_trait & BlendPixelTrait) != 0) ? 4U : 3U)*
      sample_size;
  do
  {
    Image
      *next;

    size_t
      bytes_per_row;

    if (HeapOverflowSanityCheckGetSize(image->columns,channels_size,&bytes_per_row) != MagickFalse)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),CoderError,
          "MemoryAllocationFailed","`%s'",image->filename);
        status=MagickFalse;
        break;
      }
    pixel_info=AcquireVirtualMemory(bytes_per_row,image->rows*sizeof(*pixels));
    if (pixel_info == (MemoryInfo *) NULL)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),CoderError,
          "MemoryAllocationFailed","`%s'",image->filename);
        status=MagickFalse;
        break;
      }

    if (basic_info.have_animation == JXL_TRUE)
      {
        JxlBlendInfo
          alpha_blend_info;