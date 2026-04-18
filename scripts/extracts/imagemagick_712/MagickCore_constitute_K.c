// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/constitute.c
// Segment 11/15



  /*
    Read image list from a file.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  read_info=CloneImageInfo(image_info);
  *read_info->magick='\0';
  (void) SetImageOption(read_info,"filename",filename);
  (void) CopyMagickString(read_info->filename,filename,MagickPathExtent);
  (void) InterpretImageFilename(read_info,(Image *) NULL,filename,
    (int) read_info->scene,read_filename,exception);
  if (LocaleCompare(read_filename,read_info->filename) != 0)
    {
      ExceptionInfo
        *sans;

      ssize_t
        extent,
        scene;

      /*
        Images of the form image-%d.png[1-5].
      */
      sans=AcquireExceptionInfo();
      (void) SetImageInfo(read_info,0,sans);
      sans=DestroyExceptionInfo(sans);
      if (read_info->number_scenes != 0)
        {
          (void) CopyMagickString(read_filename,read_info->filename,
            MagickPathExtent);
          images=NewImageList();
          extent=(ssize_t) (read_info->scene+read_info->number_scenes);
          scene=(ssize_t) read_info->scene;
          for ( ; scene < (ssize_t) extent; scene++)
          {
            (void) InterpretImageFilename(image_info,(Image *) NULL,
              read_filename,(int) scene,read_info->filename,exception);
            image=ReadImage(read_info,exception);
            if (image == (Image *) NULL)
              continue;
            AppendImageToList(&images,image);
          }
          read_info=DestroyImageInfo(read_info);
          return(images);
        }
    }
  (void) CopyMagickString(read_info->filename,filename,MagickPathExtent);
  image=ReadImage(read_info,exception);
  read_info=DestroyImageInfo(read_info);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e a d I n l i n e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadInlineImage() reads a Base64-encoded inline image or image sequence.
%  The method returns a NULL if there is a memory shortage or if the image
%  cannot be read.  On failure, a NULL image is returned and exception
%  describes the reason for the failure.
%
%  The format of the ReadInlineImage method is:
%
%      Image *ReadInlineImage(const ImageInfo *image_info,const char *content,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o content: the image encoded in Base64.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *ReadInlineImage(const ImageInfo *image_info,
  const char *content,ExceptionInfo *exception)
{
  Image
    *image;

  ImageInfo
    *read_info;

  unsigned char
    *blob;

  size_t
    length;

  const char
    *p;

  /*
    Skip over header (e.g. data:image/gif;base64,).
  */
  image=NewImageList();
  for (p=content; (*p != ',') && (*p != '\0'); p++) ;
  if (*p == '\0')
    ThrowReaderException(CorruptImageError,"CorruptImage");
  blob=Base64Decode(++p,&length);
  if (length == 0)
    {
      blob=(unsigned char *) RelinquishMagickMemory(blob);
      ThrowReaderException(CorruptImageError,"CorruptImage");
    }
  read_info=CloneImageInfo(image_info);
  (void) SetImageInfoProgressMonitor(read_info,(MagickProgressMonitor) NULL,
    (void *) NULL);
  *read_info->filename='\0';
  *read_info->magick='\0';
  for (p=content; (*p != '/') && (*p != '\0'); p++) ;
  if (*p != '\0')
    {
      char
        *q;

      ssize_t
        i;