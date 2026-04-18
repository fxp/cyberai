// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 24/24



      length=strlen(value);
      for (i=0; i < (ssize_t) length; i+=65533L)
        jpeg_write_marker(jpeg_info,JPEG_COM,(unsigned char *) value+i,
          (unsigned int) MagickMin((size_t) strlen(value+i),65533L));
    }
  if (image->profiles != (void *) NULL)
    WriteProfiles(jpeg_info,image,exception);
  /*
    Convert MIFF to JPEG raster pixels.
  */
  bytes_per_pixel=(jpeg_info->data_precision+7)/8;
  memory_info=AcquireVirtualMemory((size_t) image->columns,
    (size_t) (jpeg_info->input_components*bytes_per_pixel));
  if (memory_info == (MemoryInfo *) NULL)
    ThrowJPEGWriterException(ResourceLimitError,"MemoryAllocationFailed");
  jpeg_pixels=(JSAMPLE *) GetVirtualMemoryBlob(memory_info);
  if (setjmp(client_info->error_recovery) != 0)
    {
      jpeg_destroy_compress(jpeg_info);
      client_info=(JPEGClientInfo *) RelinquishMagickMemory(client_info);
      if (memory_info != (MemoryInfo *) NULL)
        memory_info=RelinquishVirtualMemory(memory_info);
      (void) CloseBlob(image);
      if (jps_image != (Image *) NULL)
        jps_image=DestroyImage(jps_image);
      return(MagickFalse);
    }
  range=GetQuantumRange(jpeg_info->data_precision);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *p;

    JDIMENSION
      number_scanlines = 0;

    ssize_t
      x;

    /*
      Transfer image pixel to JPEG buffer.
    */
    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    q=jpeg_pixels;
    switch (jpeg_info->input_components)
    {
      case 1:
      {
        /*
          Grayscale.
        */
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          JPEGSetSample(jpeg_info,ClampToQuantum(GetPixelLuma(image,p)),range,
            q);
          q+=(ptrdiff_t) bytes_per_pixel;
          p+=(ptrdiff_t) GetPixelChannels(image);
        }
        break;
      }
      case 4:
      {
        /*
          CMYK.
        */
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          /*
            Convert DirectClass packets to contiguous CMYK scanlines.
          */
          JPEGSetSample(jpeg_info,(Quantum) ((QuantumRange-
            GetPixelCyan(image,p))),range,q);
          q+=(ptrdiff_t) bytes_per_pixel;
          JPEGSetSample(jpeg_info,(Quantum) ((QuantumRange-
            GetPixelMagenta(image,p))),range,q);
          q+=(ptrdiff_t) bytes_per_pixel;
          JPEGSetSample(jpeg_info,(Quantum) ((QuantumRange-
            GetPixelYellow(image,p))),range,q);
          q+=(ptrdiff_t) bytes_per_pixel;
          JPEGSetSample(jpeg_info,(Quantum) ((QuantumRange-
            GetPixelBlack(image,p))),range,q);
          q+=(ptrdiff_t) bytes_per_pixel;
          p+=(ptrdiff_t) GetPixelChannels(image);
        }
        break;
      }
      default:
      {
        /*
          RGB || YCC.
        */
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          JPEGSetSample(jpeg_info,GetPixelRed(image,p),range,q);
          q+=(ptrdiff_t) bytes_per_pixel;
          JPEGSetSample(jpeg_info,GetPixelGreen(image,p),range,q);
          q+=(ptrdiff_t) bytes_per_pixel;
          JPEGSetSample(jpeg_info,GetPixelBlue(image,p),range,q);
          q+=(ptrdiff_t) bytes_per_pixel;
          p+=(ptrdiff_t) GetPixelChannels(image);
        }
        break;
      }
    }
    /*
      Compress one JPEG scanline.
    */
    if (jpeg_info->data_precision > 12)
      {
#if defined(MAGICKCORE_HAVE_JPEG16_WRITE_SCANLINES)
        number_scanlines=jpeg16_write_scanlines(jpeg_info,(J16SAMPROW *)
          &jpeg_pixels,1);
#endif
      }
    else if (jpeg_info->data_precision > 8)
      {
#if defined(MAGICKCORE_HAVE_JPEG12_WRITE_SCANLINES)
        number_scanlines=jpeg12_write_scanlines(jpeg_info,(J12SAMPROW *)
          &jpeg_pixels,1);
#endif
      }
    else
      {
        number_scanlines=jpeg_write_scanlines(jpeg_info,(JSAMPROW *)
          &jpeg_pixels,1);
      }
    if (number_scanlines != 1)
      (void) ThrowMagickException(exception,GetMagickModule(),
        CorruptImageError,"AnErrorHasOccurredWritingToFile","`%s'",
        image->filename);
    status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
      image->rows);
    if (status == MagickFalse)
      break;
  }
  /*
    Relinquish resources.
  */
  jpeg_finish_compress(jpeg_info);
  jpeg_destroy_compress(jpeg_info);
  client_info=(JPEGClientInfo *) RelinquishMagickMemory(client_info);
  memory_info=RelinquishVirtualMemory(memory_info);
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  if (jps_image != (Image *) NULL)
    jps_image=DestroyImage(jps_image);
  return(status);
}

static MagickBooleanType WriteJPEGImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  struct jpeg_compress_struct
    jpeg_info;

  return(WriteJPEGImage_(image_info,image,&jpeg_info,exception));
}
#endif
