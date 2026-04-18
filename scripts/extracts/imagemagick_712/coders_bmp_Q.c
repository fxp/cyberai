// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 17/24



  entry=AcquireMagickInfo("BMP","BMP","Microsoft Windows bitmap image");
  entry->decoder=(DecodeImageHandler *) ReadBMPImage;
  entry->encoder=(EncodeImageHandler *) WriteBMPImage;
  entry->magick=(IsImageFormatHandler *) IsBMP;
  entry->flags^=CoderAdjoinFlag;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->mime_type=ConstantString("image/bmp");
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("BMP","BMP2","Microsoft Windows bitmap image (V2)");
  entry->decoder=(DecodeImageHandler *) ReadBMPImage;
  entry->encoder=(EncodeImageHandler *) WriteBMPImage;
  entry->magick=(IsImageFormatHandler *) IsBMP;
  entry->flags^=CoderAdjoinFlag;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->mime_type=ConstantString("image/bmp");
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("BMP","BMP3","Microsoft Windows bitmap image (V3)");
  entry->decoder=(DecodeImageHandler *) ReadBMPImage;
  entry->encoder=(EncodeImageHandler *) WriteBMPImage;
  entry->magick=(IsImageFormatHandler *) IsBMP;
  entry->flags^=CoderAdjoinFlag;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->mime_type=ConstantString("image/bmp");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r B M P I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterBMPImage() removes format registrations made by the
%  BMP module from the list of supported formats.
%
%  The format of the UnregisterBMPImage method is:
%
%      UnregisterBMPImage(void)
%
*/
ModuleExport void UnregisterBMPImage(void)
{
  (void) UnregisterMagickInfo("BMP");
  (void) UnregisterMagickInfo("BMP2");
  (void) UnregisterMagickInfo("BMP3");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e B M P I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBMPImage() writes an image in Microsoft Windows bitmap encoded
%  image format, version 3 for Windows or (if the image has a matte channel)
%  version 4.
%
%  The format of the WriteBMPImage method is:
%
%      MagickBooleanType WriteBMPImage(const ImageInfo *image_info,
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
static MagickBooleanType WriteBMPImage(const ImageInfo *image_info,Image *image,
  ExceptionInfo *exception)
{
  BMPInfo
    bmp_info;

  BMPSubtype
    bmp_subtype;

  const char
    *option;

  const Quantum
    *p;

  const StringInfo
    *profile = (const StringInfo *) NULL;

  MagickBooleanType
    have_color_info,
    status;

  MagickOffsetType
    profile_data,
    profile_size,
    profile_size_pad,
    scene;

  MemoryInfo
    *pixel_info;

  size_t
    bytes_per_line,
    extent,
    number_scenes,
    type;

  ssize_t
    i,
    x,
    y;

  unsigned char
    *bmp_data,
    *pixels,
    *q;