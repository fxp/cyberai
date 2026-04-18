// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/constitute.c
// Segment 12/15



      /*
        Extract media type.
      */
      if (LocaleNCompare(++p,"x-",2) == 0)
        p+=(ptrdiff_t) 2;
      (void) CopyMagickString(read_info->filename,"data.",MagickPathExtent);
      q=read_info->filename+5;
      for (i=0; (*p != ';') && (*p != '\0') && (i < (MagickPathExtent-6)); i++)
        *q++=(*p++);
      *q++='\0';
    }
  image=BlobToImage(read_info,blob,length,exception);
  blob=(unsigned char *) RelinquishMagickMemory(blob);
  read_info=DestroyImageInfo(read_info);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteImage() writes an image or an image sequence to a file or file handle.
%  If writing to a file is on disk, the name is defined by the filename member
%  of the image structure.  WriteImage() returns MagickFalse is there is a
%  memory shortage or if the image cannot be written.  Check the exception
%  member of image to determine the cause for any failure.
%
%  The format of the WriteImage method is:
%
%      MagickBooleanType WriteImage(const ImageInfo *image_info,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType WriteImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent];

  const char
    *option;

  const DelegateInfo
    *delegate_info;

  const MagickInfo
    *magick_info;

  EncodeImageHandler
    *encoder;

  ExceptionInfo
    *sans_exception;

  ImageInfo
    *write_info;

  MagickBooleanType
    status,
    temporary;

  /*
    Determine image type from filename prefix or suffix (e.g. image.jpg).
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  sans_exception=AcquireExceptionInfo();
  write_info=CloneImageInfo(image_info);
  (void) CopyMagickString(write_info->filename,image->filename,
    MagickPathExtent);
  (void) SetImageInfo(write_info,1,sans_exception);
  if (*write_info->magick == '\0')
    (void) CopyMagickString(write_info->magick,image->magick,MagickPathExtent);
  (void) CopyMagickString(filename,image->filename,MagickPathExtent);
  (void) CopyMagickString(image->filename,write_info->filename,
    MagickPathExtent);
  /*
    Call appropriate image writer based on image type.
  */
  magick_info=GetMagickInfo(write_info->magick,sans_exception);
  if (sans_exception->severity == PolicyError)
    magick_info=GetMagickInfo(write_info->magick,exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  if (magick_info != (const MagickInfo *) NULL)
    {
      if (GetMagickEndianSupport(magick_info) == MagickFalse)
        image->endian=UndefinedEndian;
      else
        if ((image_info->endian == UndefinedEndian) &&
            (GetMagickRawSupport(magick_info) != MagickFalse))
          {
            unsigned long
              lsb_first;