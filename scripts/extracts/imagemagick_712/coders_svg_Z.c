// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 26/39



  /*
    Open draw file.
  */
  file=(FILE *) NULL;
  unique_file=AcquireUniqueFileResource(filename);
  if (unique_file != -1)
    file=fdopen(unique_file,"w");
  if ((unique_file == -1) || (file == (FILE *) NULL))
    {
      (void) CopyMagickString(image->filename,filename,MagickPathExtent);
      ThrowFileException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        image->filename);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Parse SVG file.
  */
  svg_info=AcquireSVGInfo();
  if (svg_info == (SVGInfo *) NULL)
    {
      (void) fclose(file);
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    }
  svg_info->file=file;
  svg_info->exception=exception;
  svg_info->image=image;
  svg_info->image_info=image_info;
  svg_info->bounds.width=(double) image->columns;
  svg_info->bounds.height=(double) image->rows;
  svg_info->svgDepth=0;
  if (image_info->size != (char *) NULL)
    (void) CloneString(&svg_info->size,image_info->size);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"begin SAX");
  xmlInitParser();
  /*
    TODO: Upgrade to SAX version 2 (startElementNs/endElementNs)
  */
  xmlSAXVersion(&sax_modules,1);
  sax_modules.startElement=SVGStartElement;
  sax_modules.endElement=SVGEndElement;
  sax_modules.reference=(referenceSAXFunc) NULL;
  sax_modules.characters=SVGCharacters;
  sax_modules.ignorableWhitespace=(ignorableWhitespaceSAXFunc) NULL;
  sax_modules.processingInstruction=(processingInstructionSAXFunc) NULL;
  sax_modules.comment=SVGComment;
  sax_modules.warning=SVGWarning;
  sax_modules.error=SVGError;
  sax_modules.fatalError=SVGError;
  sax_modules.cdataBlock=SVGCharacters;
  sax_handler=(&sax_modules);
  n=ReadBlob(image,MagickPathExtent-1,message);
  message[n]='\0';
  parser=(xmlParserCtxtPtr) NULL;
  if (n > 0)
    {
      parser=xmlCreatePushParserCtxt(sax_handler,(void *) NULL,(const char *)
        message,(int) n,image->filename);
      if (parser != (xmlParserCtxtPtr) NULL)
        {
          const char *option;
          parser->_private=(SVGInfo *) svg_info;
          option = GetImageOption(image_info,"svg:parse-huge");
          if (option == (char *) NULL)
            option=GetImageOption(image_info,"svg:xml-parse-huge");  /* deprecated */
          if ((option != (char *) NULL) &&
              (IsStringTrue(option) != MagickFalse))
            (void) xmlCtxtUseOptions(parser,XML_PARSE_HUGE);
          option=GetImageOption(image_info,"svg:substitute-entities");
          if ((option != (char *) NULL) &&
              (IsStringTrue(option) != MagickFalse))
            (void) xmlCtxtUseOptions(parser,XML_PARSE_NOENT);
          while ((n=ReadBlob(image,MagickPathExtent-1,message)) != 0)
          {
            message[n]='\0';
            status=xmlParseChunk(parser,(char *) message,(int) n,0);
            if (status != 0)
              break;
          }
        }
    }
  if (parser == (xmlParserCtxtPtr) NULL)
    {
      svg_info=DestroySVGInfo(svg_info);
      (void) RelinquishUniqueFileResource(filename);
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  (void) xmlParseChunk(parser,(char *) message,0,1);
  if (parser->myDoc != (xmlDocPtr) NULL)
    {
      xmlFreeDoc(parser->myDoc);
      parser->myDoc=(xmlDocPtr) NULL;
    }
  xmlFreeParserCtxt(parser);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"end SAX");
  (void) fclose(file);
  (void) CloseBlob(image);
  image->columns=svg_info->width;
  image->rows=svg_info->height;
  if (exception->severity >= ErrorException)
    {
      svg_info=DestroySVGInfo(svg_info);
      (void) RelinquishUniqueFileResource(filename);
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  if (image_info->ping == MagickFalse)
    {
      ImageInfo
        *read_info;