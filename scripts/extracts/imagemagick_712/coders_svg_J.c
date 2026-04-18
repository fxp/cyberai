// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 10/39

 {
            (void) FormatLocaleFile(svg_info->file,"stroke-opacity \"%s\"\n",
              value);
            break;
          }
        if (LocaleCompare(keyword,"stroke-width") == 0)
          {
            (void) FormatLocaleFile(svg_info->file,"stroke-width %g\n",
              GetUserSpaceCoordinateValue(svg_info,1,value));
            break;
          }
        break;
      }
      case 't':
      case 'T':
      {
        if (LocaleCompare(keyword,"text-align") == 0)
          {
            (void) FormatLocaleFile(svg_info->file,"text-align \"%s\"\n",value);
            break;
          }
        if (LocaleCompare(keyword,"text-anchor") == 0)
          {
            (void) FormatLocaleFile(svg_info->file,"text-anchor \"%s\"\n",
              value);
            break;
          }
        if (LocaleCompare(keyword,"text-decoration") == 0)
          {
            if (LocaleCompare(value,"underline") == 0)
              (void) FormatLocaleFile(svg_info->file,"decorate underline\n");
            if (LocaleCompare(value,"line-through") == 0)
              (void) FormatLocaleFile(svg_info->file,"decorate line-through\n");
            if (LocaleCompare(value,"overline") == 0)
              (void) FormatLocaleFile(svg_info->file,"decorate overline\n");
            break;
          }
        if (LocaleCompare(keyword,"text-antialiasing") == 0)
          {
            (void) FormatLocaleFile(svg_info->file,"text-antialias %d\n",
              LocaleCompare(value,"true") == 0);
            break;
          }
        break;
      }
      default:
        break;
    }
    value=DestroyString(value);
  }
  if (units != (char *) NULL)
    units=DestroyString(units);
  if (color != (char *) NULL)
    color=DestroyString(color);
  for (i=0; tokens[i] != (char *) NULL; i++)
    tokens[i]=DestroyString(tokens[i]);
  tokens=(char **) RelinquishMagickMemory(tokens);
}

static void SVGStartElement(void *context,const xmlChar *name,
  const xmlChar **attributes)
{
#define PushGraphicContext(id) \
{ \
  if (*id == '\0') \
    (void) FormatLocaleFile(svg_info->file,"push graphic-context\n"); \
  else \
    (void) FormatLocaleFile(svg_info->file,"push graphic-context \"%s\"\n", \
      id); \
}

  char
    *color,
    background[MagickPathExtent],
    id[MagickPathExtent],
    *next_token,
    token[MagickPathExtent],
    **tokens,
    *units;

  const char
    *keyword,
    *p;

  size_t
    number_tokens;

  ssize_t
    i,
    j;

  SVGInfo
    *svg_info;

  xmlParserCtxtPtr
    parser;

  /*
    Called when an opening tag has been processed.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  SAX.startElement(%s",
    name);
  parser=(xmlParserCtxtPtr) context;
  svg_info=(SVGInfo *) parser->_private;
  if (svg_info->n >= MagickMaxRecursionDepth)
    {
      (void) ThrowMagickException(svg_info->exception,GetMagickModule(),
        DrawError,"VectorGraphicsNestedTooDeeply","`%s'",name);
      xmlStopParser((xmlParserCtxtPtr) context);
      return;
    }
  svg_info->n++;
  svg_info->scale=(double *) ResizeQuantumMemory(svg_info->scale,(size_t)
    svg_info->n+1,sizeof(*svg_info->scale));
  if (svg_info->scale == (double *) NULL)
    {
      (void) ThrowMagickException(svg_info->exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",name);
      return;
    }
  svg_info->scale[svg_info->n]=svg_info->scale[svg_info->n-1];
  color=AcquireString("none");
  units=AcquireString("userSpaceOnUse");
  *id='\0';
  *token='\0';
  *background='\0';
  if ((LocaleCompare((char *) name,"image") == 0) ||
      (LocaleCompare((char *) name,"pattern") == 0) ||
      (LocaleCompare((char *) name,"rect") == 0) ||
      (LocaleCompare((char *) name,"text") == 0) ||
      (LocaleCompare((char *) name,"use") == 0))
    {
      svg_info->bounds.x=0.0;
      svg_info->bounds.y=0.0;
    }
  if (attributes != (const xmlChar **) NULL)
    for (i=0; (attributes[i] != (const xmlChar *) NULL); i+=2)
    {
      char
        *value;