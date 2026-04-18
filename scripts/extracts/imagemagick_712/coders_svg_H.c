// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 8/39



    keyword=(char *) tokens[i];
    value=SVGEscapeString((const char *) tokens[i+1]);
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"    %s: %s",keyword,
      value);
    switch (*keyword)
    {
      case 'B':
      case 'b':
      {
        if (LocaleCompare((const char *) name,"background") == 0)
          {
            if (LocaleCompare((const char *) name,"svg") == 0)
              (void) CopyMagickString(background,value,MagickPathExtent);
            break;
          }
        break;
      }
      case 'C':
      case 'c':
      {
         if (LocaleCompare(keyword,"clip-path") == 0)
           {
             (void) FormatLocaleFile(svg_info->file,"clip-path \"%s\"\n",value);
             break;
           }
        if (LocaleCompare(keyword,"clip-rule") == 0)
          {
            (void) FormatLocaleFile(svg_info->file,"clip-rule \"%s\"\n",value);
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
            (void) FormatLocaleFile(svg_info->file,"currentColor \"%s\"\n",
              color);
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
            if (LocaleCompare(value,"#000000ff") == 0)
              (void) FormatLocaleFile(svg_info->file,"fill '#000000'\n");
            else
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
            (void) FormatLocaleFile(svg_info->file,"fill-rule \"%s\"\n",value);
            break;
          }
        if (LocaleCompare(keyword,"fill-opacity") == 0)
          {
            (void) FormatLocaleFile(svg_info->file,"fill-opacity \"%s\"\n",
              value);
            break;
          }
        if (LocaleCompare(keyword,"font") == 0)
          {
            char
              font_family[MagickPathExtent],
              font_size[MagickPathExtent],
              font_style[MagickPathExtent];