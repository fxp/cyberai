// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 35/39

s':
      case 'S':
      {
        if (LocaleCompare("scale",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            affine.sx=StringToDouble(token,&next_token);
            (void) GetNextToken(q,&q,extent,token);
            if (*token == ',')
              (void) GetNextToken(q,&q,extent,token);
            affine.sy=StringToDouble(token,&next_token);
            break;
          }
        if (LocaleCompare("skewX",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,"skewX(%s) ",
              token);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("skewY",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,"skewY(%s) ",
              token);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("stop-color",keyword) == 0)
          {
            char
              color[MagickPathExtent];

            (void) GetNextToken(q,&q,extent,token);
            (void) CopyMagickString(color,token,MagickPathExtent);
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,
              "  <stop offset=\"%s\" stop-color=\"%s\" />\n",token,color);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("stroke",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,"stroke:%s;",
              token);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("stroke-antialias",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,
              "stroke-antialias:%s;",token);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("stroke-dasharray",keyword) == 0)
          {
            if (IsPoint(q))
              {
                ssize_t
                  k;