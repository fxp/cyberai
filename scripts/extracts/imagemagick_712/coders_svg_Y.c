// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 25/39



  /*
    A comment has been parsed.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  SAX.comment(%s)",
    value);
  parser=(xmlParserCtxtPtr) context;
  svg_info=(SVGInfo *) parser->_private;
  if (svg_info->comment != (char *) NULL)
    (void) ConcatenateString(&svg_info->comment,"\n");
  (void) ConcatenateString(&svg_info->comment,(const char *) value);
}

static void SVGWarning(void *,const char *,...)
  magick_attribute((__format__ (__printf__,2,3)));

static void SVGWarning(void *context,const char *format,...)
{
  char
    *message,
    reason[MagickPathExtent];

  SVGInfo
    *svg_info;

  xmlParserCtxtPtr
    parser;

  va_list
    operands;

  /**
    Display and format a warning messages, gives file, line, position and
    extra parameters.
  */
  va_start(operands,format);
  parser=(xmlParserCtxtPtr) context;
  svg_info=(SVGInfo *) parser->_private;
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  SAX.warning: ");
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),format,operands);
#if !defined(MAGICKCORE_HAVE_VSNPRINTF)
  (void) vsprintf(reason,format,operands);
#else
  (void) vsnprintf(reason,MagickPathExtent,format,operands);
#endif
  message=GetExceptionMessage(errno);
  (void) ThrowMagickException(svg_info->exception,GetMagickModule(),
    DelegateWarning,reason,"`%s`",message);
  message=DestroyString(message);
  va_end(operands);
}

static void SVGError(void *context,const char *format,...)
{
  char
    *message,
    reason[MagickPathExtent];

  SVGInfo
    *svg_info;

  xmlParserCtxtPtr
    parser;

  va_list
    operands;

  /*
    Display and format a error formats, gives file, line, position and
    extra parameters.
  */
  va_start(operands,format);
  parser=(xmlParserCtxtPtr) context;
  svg_info=(SVGInfo *) parser->_private;
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  SAX.error: ");
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),format,operands);
#if !defined(MAGICKCORE_HAVE_VSNPRINTF)
  (void) vsprintf(reason,format,operands);
#else
  (void) vsnprintf(reason,MagickPathExtent,format,operands);
#endif
  message=GetExceptionMessage(errno);
  (void) ThrowMagickException(svg_info->exception,GetMagickModule(),CoderError,
    reason,"`%s`",message);
  message=DestroyString(message);
  va_end(operands);
  xmlStopParser(parser);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static Image *RenderMSVGImage(const ImageInfo *image_info,Image *image,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent];

  FILE
    *file;

  Image
    *next;

  int
    status,
    unique_file;

  ssize_t
    n;

  SVGInfo
    *svg_info;

  unsigned char
    message[MagickPathExtent];

  xmlSAXHandler
    sax_modules;

  xmlSAXHandlerPtr
    sax_handler;

  xmlParserCtxtPtr
    parser;