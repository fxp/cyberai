// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 9/24



  /*
    Open image file.
  */
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  if (*offset != 0)
    (void) SeekBlob(image,*offset,SEEK_SET);
  /*
    Verify that file size large enough to contain a JPEG datastream.
  */
  if (GetBlobSize(image) < 107)
    ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
  /*
    Initialize JPEG parameters.
  */
  client_info=(JPEGClientInfo *) AcquireMagickMemory(sizeof(*client_info));
  if (client_info == (JPEGClientInfo *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memset(client_info,0,sizeof(*client_info));
  (void) memset(jpeg_info,0,sizeof(*jpeg_info));
  (void) memset(&jpeg_error,0,sizeof(jpeg_error));
  (void) memset(&jpeg_progress,0,sizeof(jpeg_progress));
  jpeg_info->err=jpeg_std_error(&jpeg_error);
  jpeg_info->err->emit_message=(void (*)(j_common_ptr,int)) JPEGWarningHandler;
  jpeg_info->err->error_exit=(void (*)(j_common_ptr)) JPEGErrorHandler;
  memory_info=(MemoryInfo *) NULL;
  client_info->exception=exception;
  client_info->image=image;
  if (setjmp(client_info->error_recovery) != 0)
    {
      client_info=JPEGCleanup(jpeg_info,client_info);
      (void) CloseBlob(image);
      if (exception->severity < ErrorException)
        return(GetFirstImageInList(image));
      return(DestroyImage(image));
    }
  jpeg_info->client_data=(void *) client_info;
  jpeg_create_decompress(jpeg_info);
  max_memory_to_use=GetMaxMemoryRequest();
  if (max_memory_to_use < (size_t) LONG_MAX)
    jpeg_info->mem->max_memory_to_use=(long) max_memory_to_use;
  jpeg_progress.progress_monitor=(void (*)(j_common_ptr)) JPEGProgressHandler;
  jpeg_info->progress=(&jpeg_progress);
  JPEGSourceManager(jpeg_info,image);
  jpeg_set_marker_processor(jpeg_info,JPEG_COM,ReadComment);
  option=GetImageOption(image_info,"profile:skip");
  for (i=1; i < MaxJPEGProfiles; i++)
  {
    if (i == ICC_INDEX)
      {
        if (IsOptionMember("ICC",option) == MagickFalse)
          jpeg_set_marker_processor(jpeg_info,ICC_MARKER,ReadICCProfile);
      }
    else if (i == IPTC_INDEX)
      {
        if (IsOptionMember("IPTC",option) == MagickFalse)
          jpeg_set_marker_processor(jpeg_info,IPTC_MARKER,ReadIPTCProfile);
      }
    else if (i != 14)
      {
        /*
          Ignore APP14 as this will change the colors of the image.
        */
        if (IsOptionMember("APP",option) == MagickFalse)
          jpeg_set_marker_processor(jpeg_info,(int) (JPEG_APP0+i),
            ReadAPPProfiles);
      }
  }
  (void) jpeg_read_header(jpeg_info,TRUE);
  if (IsYCbCrCompatibleColorspace(image_info->colorspace) != MagickFalse)
    jpeg_info->out_color_space=JCS_YCbCr;
  /*
    Set image resolution.
  */
  if (jpeg_info->saw_JFIF_marker != 0)
    {
      if (jpeg_info->density_unit == 1)
        image->units=PixelsPerInchResolution;
      else if (jpeg_info->density_unit == 2)
        image->units = PixelsPerCentimeterResolution;
      if (IsAspectRatio(jpeg_info) == MagickFalse)
        {
          if (jpeg_info->X_density != 0)
            image->resolution.x=(double) jpeg_info->X_density;
          if (jpeg_info->Y_density != 0)
            image->resolution.y=(double) jpeg_info->Y_density;
        }
    }
  number_pixels=(MagickSizeType) image->columns*image->rows;
  option=GetImageOption(image_info,"jpeg:size");
  if ((option != (const char *) NULL) &&
      (jpeg_info->out_color_space != JCS_YCbCr))
    {
      double
        scale_factor;

      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;