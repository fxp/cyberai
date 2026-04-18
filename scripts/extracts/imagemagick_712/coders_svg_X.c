// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 24/39

se 's':
    {
      if (LocaleCompare((const char *) name,"stop") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"stop-color \"%s\" %s\n",
            svg_info->stop_color,svg_info->offset == (char *) NULL ? "100%" :
            svg_info->offset);
          break;
        }
      if (LocaleCompare((char *) name,"style") == 0)
        {
          char
            *keyword,
            **tokens,
            *value;

          ssize_t
            j;

          size_t
            number_tokens;

          /*
            Find style definitions in svg_info->text.
          */
          tokens=SVGKeyValuePairs(svg_info,'{','}',svg_info->text,
            &number_tokens);
          if (tokens == (char **) NULL)
            break;
          for (j=0; j < ((ssize_t) number_tokens-1); j+=2)
          {
            keyword=(char *) tokens[j];
            value=(char *) tokens[j+1];
            (void) FormatLocaleFile(svg_info->file,"push class \"%s\"\n",
              *keyword == '.' ? keyword+1 : keyword);
            SVGProcessStyleElement(svg_info,name,value);
            (void) FormatLocaleFile(svg_info->file,"pop class\n");
          }
          for (j=0; tokens[j] != (char *) NULL; j++)
            tokens[j]=DestroyString(tokens[j]);
          tokens=(char **) RelinquishMagickMemory(tokens);
          break;
        }
      if (LocaleCompare((const char *) name,"svg") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          svg_info->svgDepth--;
          break;
        }
      if (LocaleCompare((const char *) name,"symbol") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"pop symbol\n");
          break;
        }
      break;
    }
    case 'T':
    case 't':
    {
      if (LocaleCompare((const char *) name,"text") == 0)
        {
          if (*svg_info->text != '\0')
            {
              char
                *text;

              SVGStripString(MagickTrue,svg_info->text);
              text=EscapeString(svg_info->text,'\"');
              (void) FormatLocaleFile(svg_info->file,"text %g,%g \"%s\"\n",
                svg_info->text_offset.x,svg_info->text_offset.y,text);
              text=DestroyString(text);
              *svg_info->text='\0';
            }
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      if (LocaleCompare((const char *) name,"tspan") == 0)
        {
          if (*svg_info->text != '\0')
            {
              char
                *text;

              text=EscapeString(svg_info->text,'\"');
              (void) FormatLocaleFile(svg_info->file,"text %g,%g \"%s\"\n",
                svg_info->bounds.x,svg_info->bounds.y,text);
              text=DestroyString(text);
              *svg_info->text='\0';
            }
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      if (LocaleCompare((const char *) name,"title") == 0)
        {
          if (*svg_info->text == '\0')
            break;
          (void) CloneString(&svg_info->title,svg_info->text);
          *svg_info->text='\0';
          break;
        }
      break;
    }
    case 'U':
    case 'u':
    {
      if (LocaleCompare((char *) name,"use") == 0)
        {
          if ((svg_info->bounds.x != 0.0) || (svg_info->bounds.y != 0.0))
            (void) FormatLocaleFile(svg_info->file,"translate %g,%g\n",
              svg_info->bounds.x,svg_info->bounds.y);
          (void) FormatLocaleFile(svg_info->file,"use \"url(%s)\"\n",
            svg_info->url);
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      break;
    }
    default:
      break;
  }
  *svg_info->text='\0';
  (void) memset(&svg_info->element,0,sizeof(svg_info->element));
  (void) memset(&svg_info->segment,0,sizeof(svg_info->segment));
  svg_info->n--;
}

static void SVGCharacters(void *context,const xmlChar *c,int length)
{
  char
    *text;

  SVGInfo
    *svg_info;

  xmlParserCtxtPtr
    parser;

  /*
    Receiving some characters from the parser.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.characters(%s,%.20g)",c,(double) length);
  parser=(xmlParserCtxtPtr) context;
  svg_info=(SVGInfo *) parser->_private;
  text=(char *) AcquireQuantumMemory((size_t) length+1,sizeof(*text));
  if (text == (char *) NULL)
    return;
  memcpy(text,c,length);
  text[length] = '\0';
  SVGStripString(MagickFalse,text);
  if (svg_info->text == (char *) NULL)
    svg_info->text=text;
  else
    {
      (void) ConcatenateString(&svg_info->text,text);
      text=DestroyString(text);
    }
}

static void SVGComment(void *context,const xmlChar *value)
{
  SVGInfo
    *svg_info;

  xmlParserCtxtPtr
    parser;