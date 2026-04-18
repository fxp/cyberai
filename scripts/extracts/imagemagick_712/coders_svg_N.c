// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 14/39



              dy=GetUserSpaceCoordinateValue(svg_info,-1,value);
              svg_info->bounds.y+=dy;
              svg_info->text_offset.y+=dy;
              if (LocaleCompare((char *) name,"text") == 0)
                (void) FormatLocaleFile(svg_info->file,"translate 0.0,%g\n",dy);
              break;
            }
          break;
        }
        case 'F':
        case 'f':
        {
          if (LocaleCompare(keyword,"fill") == 0)
            {
              if (LocaleCompare(value,"currentColor") == 0)
                {
                  (void) FormatLocaleFile(svg_info->file,"fill \"%s\"\n",color);
                  break;
                }
              (void) FormatLocaleFile(svg_info->file,"fill \"%s\"\n",value);
              break;
            }
          if (LocaleCompare(keyword,"fillcolor") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"fill \"%s\"\n",value);
              break;
            }
          if (LocaleCompare(keyword,"fill-rule") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"fill-rule \"%s\"\n",
                value);
              break;
            }
          if (LocaleCompare(keyword,"fill-opacity") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"fill-opacity \"%s\"\n",
                value);
              break;
            }
          if (LocaleCompare(keyword,"font-family") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"font-family \"%s\"\n",
                value);
              break;
            }
          if (LocaleCompare(keyword,"font-stretch") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"font-stretch \"%s\"\n",
                value);
              break;
            }
          if (LocaleCompare(keyword,"font-style") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"font-style \"%s\"\n",
                value);
              break;
            }
          if (LocaleCompare(keyword,"font-size") == 0)
            {
              if (LocaleCompare(value,"xx-small") == 0)
                svg_info->pointsize=6.144;
              else if (LocaleCompare(value,"x-small") == 0)
                svg_info->pointsize=7.68;
              else if (LocaleCompare(value,"small") == 0)
                svg_info->pointsize=9.6;
              else if (LocaleCompare(value,"medium") == 0)
                svg_info->pointsize=12.0;
              else if (LocaleCompare(value,"large") == 0)
                svg_info->pointsize=14.4;
              else if (LocaleCompare(value,"x-large") == 0)
                svg_info->pointsize=17.28;
              else if (LocaleCompare(value,"xx-large") == 0)
                svg_info->pointsize=20.736;
              else
                svg_info->pointsize=GetUserSpaceCoordinateValue(svg_info,0,
                  value);
              (void) FormatLocaleFile(svg_info->file,"font-size %g\n",
                svg_info->pointsize);
              break;
            }
          if (LocaleCompare(keyword,"font-weight") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"font-weight \"%s\"\n",
                value);
              break;
            }
          break;
        }
        case 'G':
        case 'g':
        {
          if (LocaleCompare(keyword,"gradientTransform") == 0)
            {
              AffineMatrix
                affine,
                current,
                transform;

              GetAffineMatrix(&transform);
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  ");
              tokens=SVGKeyValuePairs(svg_info,'(',')',value,&number_tokens);
              if (tokens == (char **) NULL)
                break;
              for (j=0; j < ((ssize_t) number_tokens-1); j+=2)
              {
                char
                  *token_value;