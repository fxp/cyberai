// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 13/39



      keyword=(const char *) attributes[i];
      value=SVGEscapeString((const char *) attributes[i+1]);
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    %s = %s",keyword,value);
      switch (*keyword)
      {
        case 'A':
        case 'a':
        {
          if (LocaleCompare(keyword,"angle") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"angle %g\n",
                GetUserSpaceCoordinateValue(svg_info,0,value));
              break;
            }
          break;
        }
        case 'C':
        case 'c':
        {
          if (LocaleCompare(keyword,"class") == 0)
            {
              p=value;
              (void) GetNextToken(p,&p,MagickPathExtent,token);
              if (*token == ',')
                (void) GetNextToken(p,&p,MagickPathExtent,token);
              if (*token != '\0')
                (void) FormatLocaleFile(svg_info->file,"class \"%s\"\n",value);
              else
                (void) FormatLocaleFile(svg_info->file,"class \"none\"\n");
              break;
            }
          if (LocaleCompare(keyword,"clip-path") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"clip-path \"%s\"\n",
                value);
              break;
            }
          if (LocaleCompare(keyword,"clip-rule") == 0)
            {
              (void) FormatLocaleFile(svg_info->file,"clip-rule \"%s\"\n",
                value);
              break;
            }
          if (LocaleCompare(keyword,"clipPathUnits") == 0)
            {
              (void) CloneString(&units,value);
              (void) FormatLocaleFile(svg_info->file,"clip-units \"%s\"\n",
                value);
              break;
            }
          if (LocaleCompare(keyword,"color") == 0)
            {
              (void) CloneString(&color,value);
              break;
            }
          if (LocaleCompare(keyword,"cx") == 0)
            {
              svg_info->element.cx=
                GetUserSpaceCoordinateValue(svg_info,1,value);
              break;
            }
          if (LocaleCompare(keyword,"cy") == 0)
            {
              svg_info->element.cy=
                GetUserSpaceCoordinateValue(svg_info,-1,value);
              break;
            }
          break;
        }
        case 'D':
        case 'd':
        {
          if (LocaleCompare(keyword,"d") == 0)
            {
              (void) CloneString(&svg_info->vertices,value);
              break;
            }
          if (LocaleCompare(keyword,"dx") == 0)
            {
              double
                dx;

              dx=GetUserSpaceCoordinateValue(svg_info,1,value);
              svg_info->bounds.x+=dx;
              svg_info->text_offset.x+=dx;
              if (LocaleCompare((char *) name,"text") == 0)
                (void) FormatLocaleFile(svg_info->file,"translate %g,0.0\n",dx);
              break;
            }
          if (LocaleCompare(keyword,"dy") == 0)
            {
              double
                dy;