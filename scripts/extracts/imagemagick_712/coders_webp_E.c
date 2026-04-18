// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/webp.c
// Segment 5/11



  if (webp_status != VP8_STATUS_OK)
    switch (webp_status)
    {
      case VP8_STATUS_OUT_OF_MEMORY:
      {
        ThrowWEBPException(ResourceLimitError,"MemoryAllocationFailed");
        break;
      }
      case VP8_STATUS_INVALID_PARAM:
      {
        ThrowWEBPException(CorruptImageError,"invalid parameter");
        break;
      }
      case VP8_STATUS_BITSTREAM_ERROR:
      {
        ThrowWEBPException(CorruptImageError,"CorruptImage");
        break;
      }
      case VP8_STATUS_UNSUPPORTED_FEATURE:
      {
        ThrowWEBPException(CoderError,"DataEncodingSchemeIsNotSupported");
        break;
      }
      case VP8_STATUS_SUSPENDED:
      {
        ThrowWEBPException(CorruptImageError,"decoder suspended");
        break;
      }
      case VP8_STATUS_USER_ABORT:
      {
        ThrowWEBPException(CorruptImageError,"user abort");
        break;
      }
      case VP8_STATUS_NOT_ENOUGH_DATA:
      {
        ThrowWEBPException(CorruptImageError,"InsufficientImageDataInFile");
        break;
      }
      default:
        ThrowWEBPException(CorruptImageError,"CorruptImage");
    }

  stream=(unsigned char*) RelinquishMagickMemory(stream);
  (void) CloseBlob(image);
  return(image);
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r W E B P I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterWEBPImage() adds attributes for the WebP image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterWEBPImage method is:
%
%      size_t RegisterWEBPImage(void)
%
*/
ModuleExport size_t RegisterWEBPImage(void)
{
  char
    version[MagickPathExtent];

  MagickInfo
    *entry;