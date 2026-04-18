// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 9/39



            if (MagickSscanf(value,"%2048s %2048s %2048s",font_style,font_size,
                  font_family) != 3)
              break;
            if (GetUserSpaceCoordinateValue(svg_info,0,font_style) == 0)
              (void) FormatLocaleFile(svg_info->file,"font-style \"%s\"\n",
                style);
            else
              if (MagickSscanf(value,"%2048s %2048s",font_size,font_family) != 2)
                break;
            (void) FormatLocaleFile(svg_info->file,"font-size \"%s\"\n",
              font_size);
            (void) FormatLocaleFile(svg_info->file,"font-family \"%s\"\n",
              font_family);
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
            (void) FormatLocaleFile(svg_info->file,"font-style \"%s\"\n",value);
            break;
          }
        if (LocaleCompare(keyword,"font-size") == 0)
          {
            svg_info->pointsize=GetUserSpaceCoordinateValue(svg_info,0,value);
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
      case 'K':
      case 'k':
      {
        if (LocaleCompare(keyword,"kerning") == 0)
          {
            (void) FormatLocaleFile(svg_info->file,"kerning \"%s\"\n",value);
            break;
          }
        break;
      }
      case 'L':
      case 'l':
      {
        if (LocaleCompare(keyword,"letter-spacing") == 0)
          {
            (void) FormatLocaleFile(svg_info->file,"letter-spacing \"%s\"\n",
              value);
            break;
          }
        break;
      }
      case 'M':
      case 'm':
      {
        if (LocaleCompare(keyword,"mask") == 0)
          {
            (void) FormatLocaleFile(svg_info->file,"mask \"%s\"\n",value);
            break;
          }
        break;
      }
      case 'O':
      case 'o':
      {
        if (LocaleCompare(keyword,"offset") == 0)
          {
            (void) FormatLocaleFile(svg_info->file,"offset %g\n",
              GetUserSpaceCoordinateValue(svg_info,1,value));
            break;
          }
        if (LocaleCompare(keyword,"opacity") == 0)
          {
            (void) FormatLocaleFile(svg_info->file,"opacity \"%s\"\n",value);
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
                (void) FormatLocaleFile(svg_info->file,"stroke \"%s\"\n",color);
                break;
              }
            if (LocaleCompare(value,"#000000ff") == 0)
              (void) FormatLocaleFile(svg_info->file,"fill '#000000'\n");
            else
              (void) FormatLocaleFile(svg_info->file,
                "stroke \"%s\"\n",value);
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
            (void) FormatLocaleFile(svg_info->file,"stroke-miterlimit \"%s\"\n",
              value);
            break;
          }
        if (LocaleCompare(keyword,"stroke-opacity") == 0)
         