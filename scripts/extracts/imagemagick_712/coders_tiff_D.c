// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 4/34



  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  (void) SetMagickThreadValue(tiff_exception,exception);
  /*
    Write raw CCITT Group 4 wrapped as a TIFF image file.
  */
  unique_file=AcquireUniqueFileResource(filename);
  if (unique_file == -1)
    ThrowImageException(FileOpenError,"UnableToCreateTemporaryFile");
  (void) close_utf8(unique_file);
  tiff=TIFFOpen(filename,"w");
  if (tiff == (TIFF *) NULL)
    {
      (void) RelinquishUniqueFileResource(filename);
      ThrowImageException(FileOpenError,"UnableToCreateTemporaryFile");
    }
  TIFFSetField(tiff,TIFFTAG_IMAGEWIDTH,(uint32) image->columns);
  TIFFSetField(tiff,TIFFTAG_IMAGELENGTH,(uint32) image->rows);
  TIFFSetField(tiff,TIFFTAG_BITSPERSAMPLE,1);
  TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,1);
  TIFFSetField(tiff,TIFFTAG_PHOTOMETRIC,PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tiff,TIFFTAG_ORIENTATION,ORIENTATION_TOPLEFT);
  TIFFSetField(tiff,TIFFTAG_COMPRESSION,COMPRESSION_CCITTFAX4);
  TIFFSetField(tiff,TIFFTAG_ROWSPERSTRIP,(uint32) image->rows);
  if ((image->resolution.x > 0.0) && (image->resolution.y > 0.0))
    {
      if (image->units == PixelsPerCentimeterResolution)
        TIFFSetField(tiff,TIFFTAG_RESOLUTIONUNIT,RESUNIT_CENTIMETER);
      else
        TIFFSetField(tiff,TIFFTAG_RESOLUTIONUNIT,RESUNIT_INCH);
      TIFFSetField(tiff,TIFFTAG_XRESOLUTION,(uint32) image->resolution.x);
      TIFFSetField(tiff,TIFFTAG_YRESOLUTION,(uint32) image->resolution.y);
    }
  status=MagickTrue;
  c=ReadBlobByte(image);
  while (c != EOF)
  {
    unsigned char byte = (unsigned char) c;
    if (TIFFWriteRawStrip(tiff,0,&byte,1) < 0)
      {
        status=MagickFalse;
        break;
      }
    c=ReadBlobByte(image);
  }
  TIFFClose(tiff);
  (void) CloseBlob(image);
  image=DestroyImage(image);
  if (status == MagickFalse)
    {
      (void) RelinquishUniqueFileResource(filename);
      return((Image*) NULL);
    }
  /*
    Read TIFF image.
  */
  read_info=CloneImageInfo((ImageInfo *) NULL);
  (void) FormatLocaleString(read_info->filename,MagickPathExtent,"%s",filename);
  image=ReadTIFFImage(read_info,exception);
  read_info=DestroyImageInfo(read_info);
  if (image != (Image *) NULL)
    {
      (void) CopyMagickString(image->filename,image_info->filename,
        MagickPathExtent);
      (void) CopyMagickString(image->magick_filename,image_info->filename,
        MagickPathExtent);
      (void) CopyMagickString(image->magick,"GROUP4",MagickPathExtent);
    }
  (void) RelinquishUniqueFileResource(filename);
  return(image);
}
#endif

#if defined(MAGICKCORE_TIFF_DELEGATE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d T I F F I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadTIFFImage() reads a Tagged image file and returns it.  It allocates the
%  memory necessary for the new Image structure and returns a pointer to the
%  new image.
%
%  The format of the ReadTIFFImage method is:
%
%      Image *ReadTIFFImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline unsigned char ClampYCC(double value)
{
  value=255.0-value;
  if (value < 0.0)
    return((unsigned char)0);
  if (value > 255.0)
    return((unsigned char)255);
  return((unsigned char)(value));
}

static MagickBooleanType DecodeLabImage(Image *image,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;