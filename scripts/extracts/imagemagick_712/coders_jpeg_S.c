// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 19/24



  ssize_t
    bytes_per_pixel,
    i,
    y;

  struct jpeg_error_mgr
    jpeg_error;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(myImage != (Image *) NULL);
  assert(myImage->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  image=myImage;
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((LocaleCompare(image_info->magick,"JPS") == 0) &&
      (image->next != (Image *) NULL))
    {
      jps_image=AppendImages(image,MagickFalse,exception);
      if (jps_image != (Image *) NULL)
        image=jps_image;
    }
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      if (jps_image != (Image *) NULL)
        jps_image=DestroyImage(jps_image);
      return(status);
    }
  /*
    Initialize JPEG parameters.
  */
  client_info=(JPEGClientInfo *) AcquireMagickMemory(sizeof(*client_info));
  if (client_info == (JPEGClientInfo *) NULL)
    ThrowJPEGWriterException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memset(client_info,0,sizeof(*client_info));
  (void) memset(jpeg_info,0,sizeof(*jpeg_info));
  (void) memset(&jpeg_error,0,sizeof(jpeg_error));
  volatile_image=image;
  jpeg_info->client_data=(void *) volatile_image;
  jpeg_info->err=jpeg_std_error(&jpeg_error);
  jpeg_info->err->emit_message=(void (*)(j_common_ptr,int)) JPEGWarningHandler;
  jpeg_info->err->error_exit=(void (*)(j_common_ptr)) JPEGErrorHandler;
  client_info->exception=exception;
  client_info->image=volatile_image;
  memory_info=(MemoryInfo *) NULL;
  if (setjmp(client_info->error_recovery) != 0)
    {
      jpeg_destroy_compress(jpeg_info);
      client_info=(JPEGClientInfo *) RelinquishMagickMemory(client_info);
      (void) CloseBlob(image);
      if (jps_image != (Image *) NULL)
        jps_image=DestroyImage(jps_image);
      return(MagickFalse);
    }
  jpeg_info->client_data=(void *) client_info;
  jpeg_create_compress(jpeg_info);
  JPEGDestinationManager(jpeg_info,image);
  if ((image->columns != (unsigned int) image->columns) ||
      (image->rows != (unsigned int) image->rows))
    ThrowJPEGWriterException(ImageError,"WidthOrHeightExceedsLimit");
  jpeg_info->image_width=(unsigned int) image->columns;
  jpeg_info->image_height=(unsigned int) image->rows;
  jpeg_info->input_components=3;
  jpeg_info->data_precision=8;
  option=GetImageOption(image_info,"jpeg:high-bit-depth");
  if (IsStringTrue(option) != MagickFalse)
    {
#if defined(C_LOSSLESS_SUPPORTED)
      if (image_info->compression == LosslessJPEGCompression)
        {
#if defined(LIBJPEG_TURBO_VERSION_NUMBER) && LIBJPEG_TURBO_VERSION_NUMBER >= 3000090
          if (image_info->quality >= 100)
            jpeg_info->data_precision=(int) image->depth;
          else
#endif
          if (image->depth > 12)
            jpeg_info->data_precision=16;
          else if (image->depth > 8)
            jpeg_info->data_precision=12;
        }
      else
#endif
        {
#if defined(MAGICKCORE_HAVE_JPEG12_WRITE_SCANLINES)
          if (image->depth > 8)
            jpeg_info->data_precision=12;
#endif
        }
    }
  jpeg_info->in_color_space=JCS_RGB;
  switch (image->colorspace)
  {
    case CMYKColorspace:
    {
      jpeg_info->input_components=4;
      jpeg_info->in_color_space=JCS_CMYK;
      break;
    }
    case YCbCrColorspace:
    case Rec601YCbCrColorspace:
    case Rec709YCbCrColorspace:
    {
      jpeg_info->in_color_space=JCS_YCbCr;
      break;
    }
    case LinearGRAYColorspace:
    case GRAYColorspace:
    {
      if (image_info->type == TrueColorType)
        break;
      jpeg_info->input_components=1;
      jpeg_info->in_color_space=JCS_GRAYSCALE;
      break;
    }
    default:
    {
      if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
        (void) TransformImageColorspace(image,sRGBColorspace,exception);
      if (image_info->type == TrueColorType)
        break;
      if (IdentifyImageCoderGray(image,exception) != MagickFalse)
        {
          jpeg_info->input_components=1;
          jpeg_info->in_color_space=JCS_GRAYSCALE;
        }
      break;
    }
  }
  jpeg_set_defaults(jpeg_info);
  option=GetImageOption(image_info,"jpeg:restart-interval");
  if (option != (const char *) NULL)
    jpeg_info->restart_interval=StringToInteger(option);
  if (jpeg_info->in_color_space == JCS_CMYK)
    jpeg_set_colorspace(jpeg_info,JCS_YCCK);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "Image resolution: %.20g,%.20g",image->resolution.x,image->resolution.y);
  if ((image->resolution.x > 0) && (image->resolution.x < (double) SHRT_MAX) &&
      (image->resolution.y > 0) && (image->resolution.y < (double) SHRT_MAX))
    {
      UINT16
        x_density=(UINT16) image->resolution.x,