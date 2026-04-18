// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 27/34



  value=GetImageArtifact(image,"tiff:document");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_DOCUMENTNAME,value);
  value=GetImageArtifact(image,"tiff:hostcomputer");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_HOSTCOMPUTER,value);
  value=GetImageArtifact(image,"tiff:artist");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_ARTIST,value);
  value=GetImageArtifact(image,"tiff:timestamp");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_DATETIME,value);
  value=GetImageArtifact(image,"tiff:make");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_MAKE,value);
  value=GetImageArtifact(image,"tiff:model");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_MODEL,value);
  value=GetImageArtifact(image,"tiff:software");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_SOFTWARE,value);
  value=GetImageArtifact(image,"tiff:copyright");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_COPYRIGHT,value);
  value=GetImageArtifact(image,"kodak-33423");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,33423,value);
  value=GetImageArtifact(image,"kodak-36867");
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,36867,value);
  value=GetImageProperty(image,"label",exception);
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_PAGENAME,value);
  value=GetImageProperty(image,"comment",exception);
  if (value != (const char *) NULL)
    (void) TIFFSetField(tiff,TIFFTAG_IMAGEDESCRIPTION,value);
  value=GetImageArtifact(image,"tiff:subfiletype");
  if (value != (const char *) NULL)
    {
      if (LocaleCompare(value,"REDUCEDIMAGE") == 0)
        (void) TIFFSetField(tiff,TIFFTAG_SUBFILETYPE,FILETYPE_REDUCEDIMAGE);
      else
        if (LocaleCompare(value,"PAGE") == 0)
          (void) TIFFSetField(tiff,TIFFTAG_SUBFILETYPE,FILETYPE_PAGE);
        else
          if (LocaleCompare(value,"MASK") == 0)
            (void) TIFFSetField(tiff,TIFFTAG_SUBFILETYPE,FILETYPE_MASK);
    }
  else
    {
      uint16
        page,
        pages;

      page=(uint16) image->scene;
      pages=(uint16) GetImageListLength(image);
      if ((adjoin != MagickFalse) && (pages > 1))
        (void) TIFFSetField(tiff,TIFFTAG_SUBFILETYPE,FILETYPE_PAGE);
      (void) TIFFSetField(tiff,TIFFTAG_PAGENUMBER,page,pages);
    }
}

static MagickBooleanType WriteTIFFChannels(Image *image,TIFF *tiff,
  TIFFInfo tiff_info,QuantumInfo *quantum_info,QuantumType quantum_type,
  tsample_t sample,unsigned char *pixels,ExceptionInfo *exception)
{
  ssize_t
    y;

  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    (void) ExportQuantumPixels(image,(CacheView*)NULL,quantum_info,
      quantum_type,pixels,exception);
    if (TIFFWritePixels(tiff,&tiff_info,y,sample,image) == -1)
      return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType WriteTIFFImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  const char
    *mode,
    *option;

  CompressionType
    compression;

  EndianType
    endian_type;

  MagickBooleanType
    adjoin,
    preserve_compression,
    status;

  MagickOffsetType
    scene;

  QuantumInfo
    *quantum_info;

  QuantumType
    quantum_type;

  size_t
    extra_samples,
    number_scenes;

  ssize_t
    i;

  TIFF
    *tiff;

  TIFFInfo
    tiff_info;

  uint16
    bits_per_sample,
    compress_tag,
    endian,
    photometric,
    predictor;

  unsigned char
    *pixels;

  void
    *sans[2] = { NULL, NULL };