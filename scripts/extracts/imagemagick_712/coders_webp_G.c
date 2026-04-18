// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/webp.c
// Segment 7/11



#if WEBP_ENCODER_ABI_VERSION >= 0x0100
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      picture->progress_hook=WebPEncodeProgress;
      picture->user_data=(void *) image;
    }
#endif
  picture->width=(int) image->columns;
  picture->height=(int) image->rows;
  picture->argb_stride=(int) image->columns;
  picture->use_argb=1;
  /*
    Allocate memory for pixels.
  */
  if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
    (void) TransformImageColorspace(image,sRGBColorspace,exception);
  *memory_info=AcquireVirtualMemory(image->columns,image->rows*
    sizeof(*(picture->argb)));
  if (*memory_info == (MemoryInfo *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  picture->argb=(uint32_t *) GetVirtualMemoryBlob(*memory_info);
  /*
    Convert image to WebP raster pixels.
  */
  status=MagickFalse;
  q=picture->argb;
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      *q++=(uint32_t) (image->alpha_trait != UndefinedPixelTrait ? (uint32_t)
        ScaleQuantumToChar(GetPixelAlpha(image,p)) << 24 : 0xff000000) |
        ((uint32_t) ScaleQuantumToChar(GetPixelRed(image,p)) << 16) |
        ((uint32_t) ScaleQuantumToChar(GetPixelGreen(image,p)) << 8) |
        ((uint32_t) ScaleQuantumToChar(GetPixelBlue(image,p)));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
    status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
      image->rows);
    if (status == MagickFalse)
      break;
  }
  return(status);
}

static MagickBooleanType WriteSingleWEBPImage(const ImageInfo *image_info,
  Image *image,WebPConfig *configure,WebPMemoryWriter *writer,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  MemoryInfo
    *memory_info;

  WebPPicture
    picture;

  if (WebPPictureInit(&picture) == 0)
    ThrowWriterException(ResourceLimitError,"UnableToEncodeImageFile");
  picture.writer=WebPMemoryWrite;
  picture.custom_ptr=writer;
  status=WriteSingleWEBPPicture(image_info,image,&picture,&memory_info,
    exception);
  if (status != MagickFalse)
    {
      status=(MagickBooleanType) WebPEncode(configure,&picture);
      if (status == MagickFalse)
        (void) ThrowMagickException(exception,GetMagickModule(),
          CorruptImageError,WebPErrorCodeMessage(picture.error_code),"`%s'",
          image->filename);
  }
  if (memory_info != (MemoryInfo *) NULL)
    memory_info=RelinquishVirtualMemory(memory_info);
  WebPPictureFree(&picture);

  return(status);
}

#if defined(MAGICKCORE_WEBPMUX_DELEGATE)
static void *WebPDestroyMemoryInfo(void *memory_info)
{
  return((void *) RelinquishVirtualMemory((MemoryInfo *) memory_info));
}

static MagickBooleanType WriteAnimatedWEBPImage(const ImageInfo *image_info,
  Image *image,const WebPConfig *configure,WebPData *webp_data,
  ExceptionInfo *exception)
{
  Image
    *frame;

  LinkedListInfo
    *memory_info_list;

  MagickBooleanType
    status;

  MemoryInfo
    *memory_info;

  size_t
    effective_delta,
    frame_timestamp;

  WebPAnimEncoder
    *enc;

  WebPAnimEncoderOptions
    enc_options;

  WebPPicture
    picture;