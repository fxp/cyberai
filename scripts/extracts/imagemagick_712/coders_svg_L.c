// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 12/39

  case 'L':
    case 'l':
    {
      if (LocaleCompare((const char *) name,"line") == 0)
        {
          PushGraphicContext(id);
          break;
        }
      if (LocaleCompare((const char *) name,"linearGradient") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,
            "push gradient \"%s\" linear %g,%g %g,%g\n",id,
            svg_info->segment.x1,svg_info->segment.y1,svg_info->segment.x2,
            svg_info->segment.y2);
          break;
        }
      break;
    }
    case 'M':
    case 'm':
    {
      if (LocaleCompare((const char *) name,"mask") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"push mask \"%s\"\n",id);
          break;
        }
      break;
    }
    case 'P':
    case 'p':
    {
      if (LocaleCompare((const char *) name,"path") == 0)
        {
          PushGraphicContext(id);
          break;
        }
      if (LocaleCompare((const char *) name,"pattern") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,
            "push pattern \"%s\" %g,%g %g,%g\n",id,
            svg_info->bounds.x,svg_info->bounds.y,svg_info->bounds.width,
            svg_info->bounds.height);
          break;
        }
      if (LocaleCompare((const char *) name,"polygon") == 0)
        {
          PushGraphicContext(id);
          break;
        }
      if (LocaleCompare((const char *) name,"polyline") == 0)
        {
          PushGraphicContext(id);
          break;
        }
      break;
    }
    case 'R':
    case 'r':
    {
      if (LocaleCompare((const char *) name,"radialGradient") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,
            "push gradient \"%s\" radial %g,%g %g,%g %g\n",
            id,svg_info->element.cx,svg_info->element.cy,
            svg_info->element.major,svg_info->element.minor,
            svg_info->element.angle);
          break;
        }
      if (LocaleCompare((const char *) name,"rect") == 0)
        {
          PushGraphicContext(id);
          break;
        }
      break;
    }
    case 'S':
    case 's':
    {
      if (LocaleCompare((char *) name,"style") == 0)
        break;
      if (LocaleCompare((const char *) name,"svg") == 0)
        {
          svg_info->svgDepth++;
          PushGraphicContext(id);
          (void) FormatLocaleFile(svg_info->file,"compliance \"SVG\"\n");
          (void) FormatLocaleFile(svg_info->file,"fill \"black\"\n");
          (void) FormatLocaleFile(svg_info->file,"fill-opacity 1\n");
          (void) FormatLocaleFile(svg_info->file,"stroke \"none\"\n");
          (void) FormatLocaleFile(svg_info->file,"stroke-width 1\n");
          (void) FormatLocaleFile(svg_info->file,"stroke-opacity 0\n");
          (void) FormatLocaleFile(svg_info->file,"fill-rule nonzero\n");
          break;
        }
      if (LocaleCompare((const char *) name,"symbol") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"push symbol\n");
          break;
        }
      break;
    }
    case 'T':
    case 't':
    {
      if (LocaleCompare((const char *) name,"text") == 0)
        {
          PushGraphicContext(id);
          svg_info->text_offset.x=svg_info->bounds.x;
          svg_info->text_offset.y=svg_info->bounds.y;
          svg_info->bounds.x=0.0;
          svg_info->bounds.y=0.0;
          svg_info->bounds.width=0.0;
          svg_info->bounds.height=0.0;
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
                svg_info->text_offset.x,svg_info->text_offset.y,text);
              text=DestroyString(text);
              *svg_info->text='\0';
            }
          PushGraphicContext(id);
          break;
        }
      break;
    }
    case 'U':
    case 'u':
    {
      if (LocaleCompare((char *) name,"use") == 0)
        {
          PushGraphicContext(id);
          break;
        }
      break;
    }
    default:
      break;
  }
  if (attributes != (const xmlChar **) NULL)
    for (i=0; (attributes[i] != (const xmlChar *) NULL); i+=2)
    {
      char
        *value;