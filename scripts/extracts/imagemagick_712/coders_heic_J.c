// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 10/17



#if defined(MAGICKCORE_HEIC_DELEGATE)
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,14,0)
  heif_init((struct heif_init_params *) NULL);
#endif
#endif
  entry=AcquireMagickInfo("HEIC","HEIC","High Efficiency Image Format");
#if defined(MAGICKCORE_HEIC_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadHEICImage;
  if (heif_have_encoder_for_format(heif_compression_HEVC))
    entry->encoder=(EncodeImageHandler *) WriteHEICImage;
#endif
  entry->magick=(IsImageFormatHandler *) IsHEIC;
  entry->mime_type=ConstantString("image/heic");
#if defined(LIBHEIF_VERSION)
  entry->version=ConstantString(LIBHEIF_VERSION);
#endif
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags^=CoderBlobSupportFlag;
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("HEIC","HEIF","High Efficiency Image Format");
#if defined(MAGICKCORE_HEIC_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadHEICImage;
  if (heif_have_encoder_for_format(heif_compression_HEVC))
    entry->encoder=(EncodeImageHandler *) WriteHEICImage;
#endif
  entry->magick=(IsImageFormatHandler *) IsHEIC;
  entry->mime_type=ConstantString("image/heif");
#if defined(LIBHEIF_VERSION)
  entry->version=ConstantString(LIBHEIF_VERSION);
#endif
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags^=CoderBlobSupportFlag;
  (void) RegisterMagickInfo(entry);
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,7,0)
  entry=AcquireMagickInfo("HEIC","AVCI","AVC Image File Format");
#if defined(MAGICKCORE_HEIC_DELEGATE)
  if (heif_have_decoder_for_format(heif_compression_AVC))
    entry->decoder=(DecodeImageHandler *) ReadHEICImage;
  if (heif_have_encoder_for_format(heif_compression_AVC))
    entry->encoder=(EncodeImageHandler *) WriteHEICImage;
#endif
  entry->magick=(IsImageFormatHandler *) IsHEIC;
  entry->mime_type=ConstantString("image/avci");
#if defined(LIBHEIF_VERSION)
  entry->version=ConstantString(LIBHEIF_VERSION);
#endif
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags^=CoderBlobSupportFlag;
  (void) RegisterMagickInfo(entry);
#endif
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,7,0)
  entry=AcquireMagickInfo("HEIC","AVIF","AV1 Image File Format");
#if defined(MAGICKCORE_HEIC_DELEGATE)
  if (heif_have_decoder_for_format(heif_compression_AV1))
    entry->decoder=(DecodeImageHandler *) ReadHEICImage;
  if (heif_have_encoder_for_format(heif_compression_AV1))
    entry->encoder=(EncodeImageHandler *) WriteHEICImage;
#endif
  entry->magick=(IsImageFormatHandler *) IsHEIC;
  entry->mime_type=ConstantString("image/avif");
#if defined(LIBHEIF_VERSION)
  entry->version=ConstantString(LIBHEIF_VERSION);
#endif
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags^=CoderBlobSupportFlag;
  (void) RegisterMagickInfo(entry);
#endif
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r H E I C I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterHEICImage() removes format registrations made by the HEIC module
%  from the list of supported formats.
%
%  The format of the UnregisterHEICImage method is:
%
%      UnregisterHEICImage(void)
%
*/
ModuleExport void UnregisterHEICImage(void)
{
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,7,0)
  (void) UnregisterMagickInfo("AVIF");
#endif
  (void) UnregisterMagickInfo("HEIC");
  (void) UnregisterMagickInfo("HEIF");
#if defined(MAGICKCORE_HEIC_DELEGATE)
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,14,0)
  heif_deinit();
#endif
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e H E I C I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteHEICImage() writes an HEIF image using the libheif library.
%
%  The format of the WriteHEICImage method is