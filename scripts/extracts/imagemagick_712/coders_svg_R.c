// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 18/39



              angle=GetUserSpaceCoordinateValue(svg_info,0,value);
              (void) FormatLocaleFile(svg_info->file,"translate %g,%g\n",
                svg_info->bounds.x,svg_info->bounds.y);
              svg_info->bounds.x=0;
              svg_info->bounds.y=0;
              (void) FormatLocaleFile(svg_info->file,"rotate %g\n",angle);
              break;
            }
          if (LocaleCompare(keyword,"rx") == 0)
            {
              if (LocaleCompare((const char *) name,"ellipse") == 0)
                svg_info->element.major=
                  GetUserSpaceCoordinateValue(svg_info,1,value);
              else
                svg_info->radius.x=
                  GetUserSpaceCoordinateValue(svg_info,1,value);
              break;
            }
          if (LocaleCompare(keyword,"ry") == 0)
            {
              if (LocaleCompare((const char *) name,"ellipse") == 0)
                svg_info->element.minor=
                  GetUserSpaceCoordinateValue(svg_info,-1,value);
              else
                svg_info->radius.y=
                  GetUserSpaceCoordinateValue(svg_info,-1,value);
              break;
            }
          break;
        }
        case 'S':
        case 's':
        {
          if (LocaleCompare(keyword,"stop-color") == 0)
            {
              (void) CloneString(&svg_info->stop_color,value);
              break;
            }
          if (LocaleCompare(keyword,"stroke") == 0)
            {
              if (LocaleCompare(value,"currentColor") == 0)
                {
                  (void) FormatLocaleFile(svg_info->file,"stroke \"%s\"\n",
                    color);
                  break;
                }
              (void) FormatLocaleFile(svg_info->file,"stroke \"%s\"\n",value);
              break;
            }
          if (LocaleCompare(keyword,"stroke-antialiasing") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"stroke-antialias %d\n",
                LocaleCompare(value,"true") == 0);
              break;
            }
          if (LocaleCompare(keyword,"stroke-dasharray") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"stroke-dasharray %s\n",
                value);
              break;
            }
          if (LocaleCompare(keyword,"stroke-dashoffset") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"stroke-dashoffset %g\n",
                GetUserSpaceCoordinateValue(svg_info,1,value));
              break;
            }
          if (LocaleCompare(keyword,"stroke-linecap") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"stroke-linecap \"%s\"\n",
                value);
              break;
            }
          if (LocaleCompare(keyword,"stroke-linejoin") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"stroke-linejoin \"%s\"\n",
                value);
              break;
            }
          if (LocaleCompare(keyword,"stroke-miterlimit") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,
                "stroke-miterlimit \"%s\"\n",value);
              break;
            }
          if (LocaleCompare(keyword,"stroke-opacity") == 0)
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
          if (LocaleCompare(keyword,"style") == 0)
            {
              SVGProcessStyleElement(svg_info,name,value);
              break;
            }
          break;
        }
        case 'T':
        case 't':
        {
          if (LocaleCompare(keyword,"text-align") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"text-align \"%s\"\n",
                value);
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
                (void) FormatLocaleFile(svg_info->file,
                  "decorate line-through\n");
              if (LocaleCompare(value,"overline") == 0)
                (void) FormatLocaleFile(svg_info->file,"decorate overline\n");
              break;
            }
          if (LocaleCompare(keyword,"text-antialiasing") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"text-antialias %d\n",
                LocaleCompare(