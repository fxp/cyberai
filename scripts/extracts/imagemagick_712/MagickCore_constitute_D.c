// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/constitute.c
// Segment 4/15



  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  ping_info=CloneImageInfo(image_info);
  ping_info->ping=MagickTrue;
  image=ReadStream(ping_info,&PingStream,exception);
  if (image != (Image *) NULL)
    {
      ResetTimer(&image->timer);
      if (ping_info->verbose != MagickFalse)
        (void) IdentifyImage(image,stdout,MagickFalse,exception);
    }
  ping_info=DestroyImageInfo(ping_info);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i n g I m a g e s                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PingImages() pings one or more images and returns them as an image list.
%
%  The format of the PingImage method is:
%
%      Image *PingImages(ImageInfo *image_info,const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o filename: the image filename.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *PingImages(ImageInfo *image_info,const char *filename,
  ExceptionInfo *exception)
{
  char
    ping_filename[MagickPathExtent];

  Image
    *image,
    *images;

  ImageInfo
    *read_info;

  /*
    Ping image list from a file.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  (void) SetImageOption(image_info,"filename",filename);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  (void) InterpretImageFilename(image_info,(Image *) NULL,image_info->filename,
    (int) image_info->scene,ping_filename,exception);
  if (LocaleCompare(ping_filename,image_info->filename) != 0)
    {
      ExceptionInfo
        *sans;

      ssize_t
        extent,
        scene;