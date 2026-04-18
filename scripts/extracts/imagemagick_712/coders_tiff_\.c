// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 28/34



  /*
    Open TIFF file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(status);
  (void) SetMagickThreadValue(tiff_exception,exception);
  endian_type=LSBEndian;
  if (image_info->endian != UndefinedEndian)
    endian_type=image_info->endian;
  option=GetImageOption(image_info,"tiff:endian");
  if (option != (const char *) NULL)
    {
      if (LocaleNCompare(option,"msb",3) == 0)
        endian_type=MSBEndian;
      if (LocaleNCompare(option,"lsb",3) == 0)
        endian_type=LSBEndian;
    }
#if defined(TIFF_VERSION_BIG)
  if (LocaleCompare(image_info->magick,"TIFF64") == 0)
    mode=endian_type == LSBEndian ? "wl8" : "wb8";
  else
#endif
    mode=endian_type == LSBEndian ? "wl" : "wb";
  tiff=TIFFClientOpen(image->filename,mode,(thandle_t) image,TIFFReadBlob,
    TIFFWriteBlob,TIFFSeekBlob,TIFFCloseBlob,TIFFGetBlobSize,TIFFMapBlob,
    TIFFUnmapBlob);
  if (tiff == (TIFF *) NULL)
    return(MagickFalse);
  if (exception->severity > ErrorException)
    {
      TIFFClose(tiff);
      return(MagickFalse);
    }
  (void) DeleteImageProfile(image,"tiff:37724");
  scene=0;
  adjoin=image_info->adjoin;
  number_scenes=GetImageListLength(image);
  option=GetImageOption(image_info,"tiff:preserve-compression");
  preserve_compression=IsStringTrue(option);
  do
  {
    /*
      Initialize TIFF fields.
    */
    if ((image_info->type != UndefinedType) &&
        (image_info->type != OptimizeType) &&
        (image_info->type != image->type))
      (void) SetImageType(image,image_info->type,exception);
    compression=image_info->compression;
    if (preserve_compression != MagickFalse)
      compression=image->compression;
    switch (compression)
    {
      case FaxCompression:
      case Group4Compression:
      {
        if (IsImageMonochrome(image) == MagickFalse)
          {
            if (IsImageGray(image) == MagickFalse)
              (void) SetImageType(image,BilevelType,exception);
            else
              (void) SetImageDepth(image,1,exception);
          }
        image->depth=1;
        image->number_meta_channels=0;
        break;
      }
      case JPEGCompression:
      {
        (void) SetImageStorageClass(image,DirectClass,exception);
        (void) SetImageDepth(image,8,exception);
        break;
      }
      default:
        break;
    }
    quantum_info=AcquireQuantumInfo(image_info,image);
    if (quantum_info == (QuantumInfo *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    if ((image->storage_class != PseudoClass) && (image->depth >= 32) &&
        (quantum_info->format == UndefinedQuantumFormat) &&
        (IsHighDynamicRangeImage(image,exception) != MagickFalse))
      {
        status=SetQuantumFormat(image,quantum_info,FloatingPointQuantumFormat);
        if (status == MagickFalse)
          {
            quantum_info=DestroyQuantumInfo(quantum_info);
            ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
          }
      }
    option=GetImageOption(image_info,"quantum:polarity");
    if (option == (const char *) NULL)
      option=GetImageArtifact(image,"tiff:photometric");
    if (option != (const char *) NULL)
      {
        if (LocaleCompare(option,"min-is-black") == 0)
          SetQuantumMinIsWhite(quantum_info,MagickFalse);
        if (LocaleCompare(option,"min-is-white") == 0)
          SetQuantumMinIsWhite(quantum_info,MagickTrue);
      }
    if ((LocaleCompare(image_info->magick,"PTIF") == 0) &&
        (GetPreviousImageInList(image) != (Image *) NULL))
      (void) TIFFSetField(tiff,TIFFTAG_SUBFILETYPE,FILETYPE_REDUCEDIMAGE);
    if ((image->columns != (uint32) image->columns) ||
        (image->rows != (uint32) image->rows))
      ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");
    (void) TIFFSetField(tiff,TIFFTAG_IMAGELENGTH,(uint32) image->rows);
    (void) TIFFSetField(tiff,TIFFTAG_IMAGEWIDTH,(uint32) image->columns);
    switch (compression)
    {
      case FaxCompression:
      {
        compress_tag=COMPRESSION_CCITTFAX3;
        break;
      }
      case Group4Compression:
      {
        compress_tag=COMPRESSION_CCITTFAX4;
        break;
      }
#if defined(COMPRESSION_JBIG)
      case JBIG1Compression:
      {
        compress_tag=COMPRESSION_JBIG;
        break;
      }
#endif
      case JPEGCompression:
      {
        compress_tag=COMPRESSION_JPEG;
        break;
      }
#if defined(COMPRESSION_LERC)
      case LERCCompression:
      {
        compress_tag=COMPRESSION_LERC;
        break;
      }
#endif
#if d