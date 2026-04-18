// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 9/30



  entry=AcquireMagickInfo("PDF","AI","Adobe Illustrator CS2");
  entry->decoder=(DecodeImageHandler *) ReadPDFImage;
  entry->encoder=(EncodeImageHandler *) WritePDFImage;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags|=CoderEncoderSeekableStreamFlag;
  entry->flags^=CoderAdjoinFlag;
  entry->flags^=CoderBlobSupportFlag;
  entry->mime_type=ConstantString("application/pdf");
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("PDF","EPDF",
    "Encapsulated Portable Document Format");
  entry->decoder=(DecodeImageHandler *) ReadPDFImage;
  entry->encoder=(EncodeImageHandler *) WritePDFImage;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags|=CoderEncoderSeekableStreamFlag;
  entry->flags^=CoderAdjoinFlag;
  entry->flags^=CoderBlobSupportFlag;
  entry->mime_type=ConstantString("application/pdf");
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("PDF","PDF","Portable Document Format");
  entry->decoder=(DecodeImageHandler *) ReadPDFImage;
  entry->encoder=(EncodeImageHandler *) WritePDFImage;
  entry->magick=(IsImageFormatHandler *) IsPDF;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags|=CoderEncoderSeekableStreamFlag;
  entry->flags^=CoderBlobSupportFlag;
  entry->mime_type=ConstantString("application/pdf");
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("PDF","PDFA","Portable Document Archive Format");
  entry->decoder=(DecodeImageHandler *) ReadPDFImage;
  entry->encoder=(EncodeImageHandler *) WritePDFImage;
  entry->magick=(IsImageFormatHandler *) IsPDF;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags|=CoderEncoderSeekableStreamFlag;
  entry->flags^=CoderBlobSupportFlag;
  entry->mime_type=ConstantString("application/pdf");
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("PDF","POCKETMOD","Pocketmod Personal Organizer");
  entry->decoder=(DecodeImageHandler *) ReadPDFImage;
  entry->encoder=(EncodeImageHandler *) WritePOCKETMODImage;
  entry->format_type=ImplicitFormatType;
  entry->flags|=CoderDecoderSeekableStreamFlag;
  entry->flags|=CoderEncoderSeekableStreamFlag;
  entry->flags^=CoderBlobSupportFlag;
  entry->mime_type=ConstantString("application/pdf");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P D F I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterPDFImage() removes format registrations made by the
%  PDF module from the list of supported formats.
%
%  The format of the UnregisterPDFImage method is:
%
%      UnregisterPDFImage(void)
%
*/
ModuleExport void UnregisterPDFImage(void)
{
  (void) UnregisterMagickInfo("AI");
  (void) UnregisterMagickInfo("EPDF");
  (void) UnregisterMagickInfo("PDF");
  (void) UnregisterMagickInfo("PDFA");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P D F I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WritePDFImage() writes an image in the Portable Document image
%  format.
%
%  The format of the WritePDFImage method is:
%
%      MagickBooleanType WritePDFImage(const ImageInfo *image_info,
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

static char *EscapeParenthesis(const char *source)
{
  char
    *destination;

  char
    *q;

  const char
    *p;

  size_t
    length;