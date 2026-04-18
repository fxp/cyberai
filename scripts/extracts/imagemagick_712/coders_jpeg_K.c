// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 11/24



      colors=(size_t) GetQuantumRange((size_t) jpeg_info->data_precision)+1;
      if (AcquireImageColormap(image,colors,exception) == MagickFalse)
        {
          client_info=JPEGCleanup(jpeg_info,client_info);
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        }
    }
  if (image->debug != MagickFalse)
    {
      if (image->interlace != NoInterlace)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Interlace: progressive");
      else
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Interlace: nonprogressive");
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Data precision: %d",
        (int) jpeg_info->data_precision);
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Geometry: %dx%d",
        (int) jpeg_info->output_width,(int) jpeg_info->output_height);
    }
  JPEGSetImageQuality(jpeg_info,image);
  JPEGSetImageSamplingFactor(jpeg_info,image,exception);
  (void) FormatLocaleString(value,MagickPathExtent,"%.20g",(double)
    jpeg_info->out_color_space);
  (void) SetImageProperty(image,"jpeg:colorspace",value,exception);
#if defined(D_ARITH_CODING_SUPPORTED)
  if (jpeg_info->arith_code == TRUE)
    (void) SetImageProperty(image,"jpeg:arithmetic-coding","true",exception);
#endif
  JPEGDestroyImageProfiles(client_info);
  *offset=TellBlob(image);
  if (image_info->ping != MagickFalse)
    {
      client_info=JPEGCleanup(jpeg_info,client_info);
      (void) CloseBlob(image);
      return(GetFirstImageInList(image));
    }
  status=SetImageExtent(image,image->columns,image->rows,exception);
  if (status == MagickFalse)
    {
      client_info=JPEGCleanup(jpeg_info,client_info);
      return(DestroyImageList(image));
    }
  (void) jpeg_start_decompress(jpeg_info);
  if ((jpeg_info->output_components != 1) &&
      (jpeg_info->output_components != 3) &&
      (jpeg_info->output_components != 4))
    {
      client_info=JPEGCleanup(jpeg_info,client_info);
      ThrowReaderException(CorruptImageError,"ImageTypeNotSupported");
    }
  bytes_per_pixel=((size_t) jpeg_info->data_precision+7)/8;
  memory_info=AcquireVirtualMemory((size_t) image->columns,
    (size_t) jpeg_info->output_components*bytes_per_pixel);
  if (memory_info == (MemoryInfo *) NULL)
    {
      client_info=JPEGCleanup(jpeg_info,client_info);
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    }
  jpeg_pixels=(JSAMPLE *) GetVirtualMemoryBlob(memory_info);
  (void) memset(jpeg_pixels,0,(size_t) (image->columns*
    (size_t) jpeg_info->output_components*sizeof(*jpeg_pixels)));
  /*
    Convert JPEG pixels to pixel packets.
  */
  if (setjmp(client_info->error_recovery) != 0)
    {
      if (memory_info != (MemoryInfo *) NULL)
        memory_info=RelinquishVirtualMemory(memory_info);
      client_info=JPEGCleanup(jpeg_info,client_info);
      (void) CloseBlob(image);
      number_pixels=(MagickSizeType) image->columns*image->rows;
      if (number_pixels != 0)
        return(GetFirstImageInList(image));
      return(DestroyImage(image));
    }
  if (jpeg_info->quantize_colors != 0)
    {
      image->colors=(size_t) jpeg_info->actual_number_of_colors;
      if (jpeg_info->out_color_space == JCS_GRAYSCALE)
        for (i=0; i < (ssize_t) image->colors; i++)
        {
          image->colormap[i].red=(double) ScaleCharToQuantum(
            jpeg_info->colormap[0][i]);
          image->colormap[i].green=image->colormap[i].red;
          image->colormap[i].blue=image->colormap[i].red;
          image->colormap[i].alpha=(MagickRealType) OpaqueAlpha;
        }
      else
        for (i=0; i < (ssize_t) image->colors; i++)
        {
          image->colormap[i].red=(double) ScaleCharToQuantum(
            jpeg_info->colormap[0][i]);
          image->colormap[i].green=(double) ScaleCharToQuantum(
            jpeg_info->colormap[1][i]);
          image->colormap[i].blue=(double) ScaleCharToQuantum(
            jpeg_info->colormap[2][i]);
          image->colormap[i].alpha=(MagickRealType) OpaqueAlpha;
        }
    }
  range=GetQuantumRange(jpeg_info->data_precision);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    JDIMENSION
      number_scanlines = 0;

    Quantum
      *magick_restrict q;

    ssize_t
      x;