// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 13/18



  entry=AcquireMagickInfo("GIF","GIF",
    "CompuServe graphics interchange format");
  entry->decoder=(DecodeImageHandler *) ReadGIFImage;
  entry->encoder=(EncodeImageHandler *) WriteGIFImage;
  entry->magick=(IsImageFormatHandler *) IsGIF;
  entry->mime_type=ConstantString("image/gif");
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("GIF","GIF87",
    "CompuServe graphics interchange format");
  entry->decoder=(DecodeImageHandler *) ReadGIFImage;
  entry->encoder=(EncodeImageHandler *) WriteGIFImage;
  entry->magick=(IsImageFormatHandler *) IsGIF;
  entry->flags^=CoderAdjoinFlag;
  entry->version=ConstantString("version 87a");
  entry->mime_type=ConstantString("image/gif");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r G I F I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterGIFImage() removes format registrations made by the
%  GIF module from the list of supported formats.
%
%  The format of the UnregisterGIFImage method is:
%
%      UnregisterGIFImage(void)
%
*/
ModuleExport void UnregisterGIFImage(void)
{
  (void) UnregisterMagickInfo("GIF");
  (void) UnregisterMagickInfo("GIF87");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e G I F I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteGIFImage() writes an image to a file in the Compuserve Graphics
%  image format.
%
%  The format of the WriteGIFImage method is:
%
%      MagickBooleanType WriteGIFImage(const ImageInfo *image_info,
%        Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image_info: the image info.
%
%    o image:  The image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType WriteGIFImage(const ImageInfo *image_info,Image *image,
  ExceptionInfo *exception)
{
  ImageInfo
    *write_info;

  int
    c;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  RectangleInfo
    page;

  size_t
    bits_per_pixel,
    delay,
    length,
    number_scenes,
    one;

  ssize_t
    i,
    j,
    opacity;

  unsigned char
    *colormap,
    *global_colormap,
    *q;