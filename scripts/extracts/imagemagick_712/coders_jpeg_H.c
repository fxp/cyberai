// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 8/24



  switch (jpeg_info->out_color_space)
  {
    case JCS_CMYK:
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Colorspace: CMYK");
      (void) FormatLocaleString(sampling_factor,MagickPathExtent,
        "%dx%d,%dx%d,%dx%d,%dx%d",jpeg_info->comp_info[0].h_samp_factor,
        jpeg_info->comp_info[0].v_samp_factor,
        jpeg_info->comp_info[1].h_samp_factor,
        jpeg_info->comp_info[1].v_samp_factor,
        jpeg_info->comp_info[2].h_samp_factor,
        jpeg_info->comp_info[2].v_samp_factor,
        jpeg_info->comp_info[3].h_samp_factor,
        jpeg_info->comp_info[3].v_samp_factor);
      break;
    }
    case JCS_GRAYSCALE:
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "Colorspace: GRAYSCALE");
      (void) FormatLocaleString(sampling_factor,MagickPathExtent,"%dx%d",
        jpeg_info->comp_info[0].h_samp_factor,
        jpeg_info->comp_info[0].v_samp_factor);
      break;
    }
    case JCS_RGB:
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Colorspace: RGB");
      (void) FormatLocaleString(sampling_factor,MagickPathExtent,
        "%dx%d,%dx%d,%dx%d",jpeg_info->comp_info[0].h_samp_factor,
        jpeg_info->comp_info[0].v_samp_factor,
        jpeg_info->comp_info[1].h_samp_factor,
        jpeg_info->comp_info[1].v_samp_factor,
        jpeg_info->comp_info[2].h_samp_factor,
        jpeg_info->comp_info[2].v_samp_factor);
      break;
    }
    default:
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Colorspace: %d",
        jpeg_info->out_color_space);
      (void) FormatLocaleString(sampling_factor,MagickPathExtent,
        "%dx%d,%dx%d,%dx%d,%dx%d",jpeg_info->comp_info[0].h_samp_factor,
        jpeg_info->comp_info[0].v_samp_factor,
        jpeg_info->comp_info[1].h_samp_factor,
        jpeg_info->comp_info[1].v_samp_factor,
        jpeg_info->comp_info[2].h_samp_factor,
        jpeg_info->comp_info[2].v_samp_factor,
        jpeg_info->comp_info[3].h_samp_factor,
        jpeg_info->comp_info[3].v_samp_factor);
      break;
    }
  }
  (void) SetImageProperty(image,"jpeg:sampling-factor",sampling_factor,
    exception);
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Sampling Factors: %s",
    sampling_factor);
}

static JPEGClientInfo *JPEGCleanup(struct jpeg_decompress_struct *jpeg_info,
  JPEGClientInfo *client_info)
{
  size_t
    i;

  if (client_info != (JPEGClientInfo *) NULL)
    {
      for (i=0; i < MaxJPEGProfiles; i++)
      {
        if (client_info->profiles[i] != (StringInfo *) NULL)
          client_info->profiles[i]=DestroyStringInfo(client_info->profiles[i]);
      }
      client_info=(JPEGClientInfo *) RelinquishMagickMemory(client_info);
    }
  jpeg_destroy_decompress(jpeg_info);
  return(client_info);
}

static void JPEGDestroyImageProfiles(JPEGClientInfo *client_info)
{
  ssize_t
    i;

  StringInfo
    *profile;

  for (i=0; i < MaxJPEGProfiles; i++)
  {
    profile=client_info->profiles[i];
    if (profile == (StringInfo *) NULL)
      continue;
    client_info->profiles[i]=DestroyStringInfo(client_info->profiles[i]);
  }
}

static Image *ReadOneJPEGImage(const ImageInfo *image_info,
  struct jpeg_decompress_struct *jpeg_info,MagickOffsetType *offset,
  ExceptionInfo *exception)
{
  char
    value[MagickPathExtent];

  const char
    *dct_method,
    *option;

  Image
    *image;

  JPEGClientInfo
    *client_info = (JPEGClientInfo *) NULL;

  JSAMPLE
    *volatile jpeg_pixels,
    *p;

  MagickBooleanType
    status;

  MagickSizeType
    number_pixels;

  MemoryInfo
    *memory_info;

  QuantumAny
    range;

  size_t
    bytes_per_pixel,
    max_memory_to_use;

  ssize_t
    i,
    y;

  struct jpeg_error_mgr
    jpeg_error;

  struct jpeg_progress_mgr
    jpeg_progress;