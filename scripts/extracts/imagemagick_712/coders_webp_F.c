// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/webp.c
// Segment 6/11



  *version='\0';
  entry=AcquireMagickInfo("WEBP","WEBP","WebP Image Format");
#if defined(MAGICKCORE_WEBP_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadWEBPImage;
  entry->encoder=(EncodeImageHandler *) WriteWEBPImage;
  (void) FormatLocaleString(version,MagickPathExtent,"libwebp %d.%d.%d [%04X]",
    (WebPGetEncoderVersion() >> 16) & 0xff,
    (WebPGetEncoderVersion() >> 8) & 0xff,
    (WebPGetEncoderVersion() >> 0) & 0xff,WEBP_ENCODER_ABI_VERSION);
#endif
  entry->mime_type=ConstantString("image/webp");
  entry->flags|=CoderDecoderSeekableStreamFlag;
#if !defined(MAGICKCORE_WEBPMUX_DELEGATE)
  entry->flags^=CoderAdjoinFlag;
#endif
  entry->magick=(IsImageFormatHandler *) IsWEBP;
  if (*version != '\0')
    entry->version=ConstantString(version);
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r W E B P I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterWEBPImage() removes format registrations made by the WebP module
%  from the list of supported formats.
%
%  The format of the UnregisterWEBPImage method is:
%
%      UnregisterWEBPImage(void)
%
*/
ModuleExport void UnregisterWEBPImage(void)
{
  (void) UnregisterMagickInfo("WEBP");
}
#if defined(MAGICKCORE_WEBP_DELEGATE)

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e W E B P I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteWEBPImage() writes an image in the WebP image format.
%
%  The format of the WriteWEBPImage method is:
%
%      MagickBooleanType WriteWEBPImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: the image info.
%
%    o image:  The image.
%
*/

#if WEBP_ENCODER_ABI_VERSION >= 0x0100
static int WebPEncodeProgress(int percent,const WebPPicture* picture)
{
#define EncodeImageTag  "Encode/Image"

  Image
    *image;

  MagickBooleanType
    status;

  image=(Image *) picture->user_data;
  status=SetImageProgress(image,EncodeImageTag,percent-1,100);
  return(status == MagickFalse ? 0 : 1);
}
#endif

static const char * WebPErrorCodeMessage(WebPEncodingError error_code)
{
  switch (error_code)
  {
    case VP8_ENC_OK:
      return "";
    case VP8_ENC_ERROR_OUT_OF_MEMORY:
      return "out of memory";
    case VP8_ENC_ERROR_BITSTREAM_OUT_OF_MEMORY:
      return "bitstream out of memory";
    case VP8_ENC_ERROR_NULL_PARAMETER:
      return "NULL parameter";
    case VP8_ENC_ERROR_INVALID_CONFIGURATION:
      return "invalid configuration";
    case VP8_ENC_ERROR_BAD_DIMENSION:
      return "bad dimension";
    case VP8_ENC_ERROR_PARTITION0_OVERFLOW:
      return "partition 0 overflow (> 512K)";
    case VP8_ENC_ERROR_PARTITION_OVERFLOW:
      return "partition overflow (> 16M)";
    case VP8_ENC_ERROR_BAD_WRITE:
      return "bad write";
    case VP8_ENC_ERROR_FILE_TOO_BIG:
      return "file too big (> 4GB)";
#if WEBP_ENCODER_ABI_VERSION >= 0x0100
    case VP8_ENC_ERROR_USER_ABORT:
      return "user abort";
    case VP8_ENC_ERROR_LAST:
      return "error last";
#endif
  }
  return "unknown exception";
}

static MagickBooleanType WriteSingleWEBPPicture(const ImageInfo *image_info,
  Image *image,WebPPicture *picture,MemoryInfo **memory_info,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  ssize_t
    y;

  uint32_t
    *magick_restrict q;