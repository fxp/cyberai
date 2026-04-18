// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 28/39



  if (svg_tree == (SplayTreeInfo *) NULL)
    svg_tree=NewSplayTree(CompareSplayTreeString,RelinquishMagickMemory,
      (void *(*)(void *)) NULL);
  *version='\0';
#if defined(LIBXML_DOTTED_VERSION)
  (void) CopyMagickString(version,"XML " LIBXML_DOTTED_VERSION,
    MagickPathExtent);
#endif
#if defined(MAGICKCORE_RSVG_DELEGATE)
#if !GLIB_CHECK_VERSION(2,35,0)
  g_type_init();
#endif
  (void) FormatLocaleString(version,MagickPathExtent,"RSVG %d.%d.%d",
    LIBRSVG_MAJOR_VERSION,LIBRSVG_MINOR_VERSION,LIBRSVG_MICRO_VERSION);
#endif
  entry=AcquireMagickInfo("SVG","SVG","Scalable Vector Graphics");
  entry->decoder=(DecodeImageHandler *) ReadSVGImage;
  entry->encoder=(EncodeImageHandler *) WriteSVGImage;
  entry->mime_type=ConstantString("image/svg+xml");
  if (*version != '\0')
    entry->version=ConstantString(version);
  entry->magick=(IsImageFormatHandler *) IsSVG;
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("SVG","SVGZ","Compressed Scalable Vector Graphics");
#if defined(MAGICKCORE_XML_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadSVGImage;
#endif
  entry->encoder=(EncodeImageHandler *) WriteSVGImage;
  entry->mime_type=ConstantString("image/svg+xml");
  if (*version != '\0')
    entry->version=ConstantString(version);
  entry->magick=(IsImageFormatHandler *) IsSVG;
  (void) RegisterMagickInfo(entry);
#if defined(MAGICKCORE_RSVG_DELEGATE)
  entry=AcquireMagickInfo("SVG","RSVG","Librsvg SVG renderer");
  entry->decoder=(DecodeImageHandler *) ReadSVGImage;
  entry->encoder=(EncodeImageHandler *) WriteSVGImage;
  entry->flags^=CoderDecoderThreadSupportFlag;
  entry->mime_type=ConstantString("image/svg+xml");
  if (*version != '\0')
    entry->version=ConstantString(version);
  entry->magick=(IsImageFormatHandler *) IsSVG;
  (void) RegisterMagickInfo(entry);
#endif
  entry=AcquireMagickInfo("SVG","MSVG",
    "ImageMagick's own SVG internal renderer");
#if defined(MAGICKCORE_XML_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadSVGImage;
#endif
  entry->encoder=(EncodeImageHandler *) WriteSVGImage;
  entry->magick=(IsImageFormatHandler *) IsSVG;
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r S V G I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterSVGImage() removes format registrations made by the
%  SVG module from the list of supported formats.
%
%  The format of the UnregisterSVGImage method is:
%
%      UnregisterSVGImage(void)
%
*/
ModuleExport void UnregisterSVGImage(void)
{
  (void) UnregisterMagickInfo("SVGZ");
  (void) UnregisterMagickInfo("SVG");
#if defined(MAGICKCORE_RSVG_DELEGATE)
  (void) UnregisterMagickInfo("RSVG");
#endif
  (void) UnregisterMagickInfo("MSVG");
  if (svg_tree != (SplayTreeInfo *) NULL)
    svg_tree=DestroySplayTree(svg_tree);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e S V G I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteSVGImage() writes a image in the SVG - XML based W3C standard
%  format.
%
%  The format of the WriteSVGImage method is:
%
%      MagickBooleanType WriteSVGImage(const ImageInfo *image_info,
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

static void AffineToTransform(Image *image,AffineMatrix *affine)
{
  char
    transform[MagickPathExtent];