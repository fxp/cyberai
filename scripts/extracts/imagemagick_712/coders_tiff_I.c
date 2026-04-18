// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 9/34



  if (GetImageListLength(image) != 1)
    return;
  if ((image_info->number_scenes == 1) && (image_info->scene == 0))
    return;
  option=GetImageOption(image_info,"tiff:ignore-layers");
  if (option != (const char * ) NULL)
    return;
  profile=GetImageProfile(image,"tiff:37724");
  if (profile == (const StringInfo *) NULL)
    return;
  for (i=0; i < (ssize_t) profile->length-8; i++)
  {
    if (LocaleNCompare((const char *) (profile->datum+i),
        image->endian == MSBEndian ? "8BIM" : "MIB8",4) != 0)
      continue;
    i+=4;
    if ((LocaleNCompare((const char *) (profile->datum+i),
         image->endian == MSBEndian ? "Layr" : "ryaL",4) == 0) ||
        (LocaleNCompare((const char *) (profile->datum+i),
         image->endian == MSBEndian ? "LMsk" : "ksML",4) == 0) ||
        (LocaleNCompare((const char *) (profile->datum+i),
         image->endian == MSBEndian ? "Lr16" : "61rL",4) == 0) ||
        (LocaleNCompare((const char *) (profile->datum+i),
         image->endian == MSBEndian ? "Lr32" : "23rL",4) == 0))
      break;
  }
  i+=4;
  if (i >= (ssize_t) (profile->length-8))
    return;
  photoshop_profile.data=(StringInfo *) profile;
  photoshop_profile.length=profile->length;
  custom_stream=TIFFAcquireCustomStreamForReading(&photoshop_profile,exception);
  if (custom_stream == (CustomStreamInfo *) NULL)
    return;
  layers=CloneImage(image,0,0,MagickTrue,exception);
  if (layers == (Image *) NULL)
    {
      custom_stream=DestroyCustomStreamInfo(custom_stream);
      return;
    }
  (void) DeleteImageProfile(layers,"tiff:37724");
  AttachCustomStream(layers->blob,custom_stream);
  SeekBlob(layers,(MagickOffsetType) i,SEEK_SET);
  InitPSDInfo(layers,&info);
  clone_info=CloneImageInfo(image_info);
  clone_info->number_scenes=0;
  sans_exception=AcquireExceptionInfo();
  (void) ReadPSDLayers(layers,clone_info,&info,sans_exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  clone_info=DestroyImageInfo(clone_info);
  DeleteImageFromList(&layers);
  if (layers != (Image *) NULL)
    {
      SetImageArtifact(image,"tiff:has-layers","true");
      AppendImageToList(&image,layers);
      while (layers != (Image *) NULL)
      {
        SetImageArtifact(layers,"tiff:has-layers","true");
        DetachBlob(layers->blob);
        layers=GetNextImageInList(layers);
      }
    }
  custom_stream=DestroyCustomStreamInfo(custom_stream);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static Image *ReadTIFFImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
#define ThrowTIFFException(severity,message) \
{ \
  if (pixel_info != (MemoryInfo *) NULL) \
    pixel_info=RelinquishVirtualMemory(pixel_info); \
  if (quantum_info != (QuantumInfo *) NULL) \
    quantum_info=DestroyQuantumInfo(quantum_info); \
  TIFFClose(tiff); \
  ThrowReaderException(severity,message); \
}

  float
    *chromaticity = (float *) NULL,
    x_position,
    y_position,
    x_resolution,
    y_resolution;

  Image
    *image;

  int
    tiff_status = 0;

  MagickBooleanType
    more_frames;

  MagickSizeType
    number_pixels;

  MagickStatusType
    status;

  MemoryInfo
    *pixel_info = (MemoryInfo *) NULL;

  QuantumInfo
    *quantum_info;

  QuantumType
    image_quantum_type;

  ssize_t
    i,
    scanline_size,
    y;

  TIFF
    *tiff;

  TIFFMethodType
    method;

  uint16
    compress_tag = 0,
    bits_per_sample = 0,
    endian = 0,
    extra_samples = 0,
    interlace = 0,
    max_sample_value = 0,
    min_sample_value = 0,
    orientation = 0,
    page = 0,
    pages = 0,
    photometric = 0,
    *sample_info = NULL,
    sample_format = 0,
    samples_per_pixel = 0,
    units = 0;

  uint32
    height,
    rows_per_strip,
    width;

  unsigned char
    *pixels;

  void
    *sans[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };