// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 34/39

Double(token,&next_token);
                svg_info.element.cx=StringToDouble(token,&next_token);
                (void) GetNextToken(q,&q,extent,token);
                if (*token == ',')
                  (void) GetNextToken(q,&q,extent,token);
                svg_info.segment.y1=StringToDouble(token,&next_token);
                svg_info.element.cy=StringToDouble(token,&next_token);
                (void) GetNextToken(q,&q,extent,token);
                if (*token == ',')
                  (void) GetNextToken(q,&q,extent,token);
                svg_info.segment.x2=StringToDouble(token,&next_token);
                svg_info.element.major=StringToDouble(token,
                  (char **) NULL);
                (void) GetNextToken(q,&q,extent,token);
                if (*token == ',')
                  (void) GetNextToken(q,&q,extent,token);
                svg_info.segment.y2=StringToDouble(token,&next_token);
                svg_info.element.minor=StringToDouble(token,
                  (char **) NULL);
                (void) FormatLocaleString(message,MagickPathExtent,
                  "<%sGradient id=\"%s\" x1=\"%g\" y1=\"%g\" x2=\"%g\" "
                  "y2=\"%g\">\n",type,name,svg_info.segment.x1,
                  svg_info.segment.y1,svg_info.segment.x2,svg_info.segment.y2);
                if (LocaleCompare(type,"radial") == 0)
                  {
                    (void) GetNextToken(q,&q,extent,token);
                    if (*token == ',')
                      (void) GetNextToken(q,&q,extent,token);
                    svg_info.element.angle=StringToDouble(token,
                      (char **) NULL);
                    (void) FormatLocaleString(message,MagickPathExtent,
                      "<%sGradient id=\"%s\" cx=\"%g\" cy=\"%g\" r=\"%g\" "
                      "fx=\"%g\" fy=\"%g\">\n",type,name,
                      svg_info.element.cx,svg_info.element.cy,
                      svg_info.element.angle,svg_info.element.major,
                      svg_info.element.minor);
                  }
                (void) WriteBlobString(image,message);
                break;
              }
            if (LocaleCompare("graphic-context",token) == 0)
              {
                n++;
                if (n == MagickMaxRecursionDepth)
                  ThrowWriterException(DrawError,
                    "VectorGraphicsNestedTooDeeply");
                if (active)
                  {
                    AffineToTransform(image,&affine);
                    active=MagickFalse;
                  }
                (void) WriteBlobString(image,"<g style=\"");
                active=MagickTrue;
              }
            if (LocaleCompare("pattern",token) == 0)
              {
                (void) GetNextToken(q,&q,extent,token);
                (void) CopyMagickString(name,token,MagickPathExtent);
                (void) GetNextToken(q,&q,extent,token);
                svg_info.bounds.x=StringToDouble(token,&next_token);
                (void) GetNextToken(q,&q,extent,token);
                if (*token == ',')
                  (void) GetNextToken(q,&q,extent,token);
                svg_info.bounds.y=StringToDouble(token,&next_token);
                (void) GetNextToken(q,&q,extent,token);
                if (*token == ',')
                  (void) GetNextToken(q,&q,extent,token);
                svg_info.bounds.width=StringToDouble(token,
                  (char **) NULL);
                (void) GetNextToken(q,&q,extent,token);
                if (*token == ',')
                  (void) GetNextToken(q,&q,extent,token);
                svg_info.bounds.height=StringToDouble(token,(char **) NULL);
                (void) FormatLocaleString(message,MagickPathExtent,
                  "<pattern id=\"%s\" x=\"%g\" y=\"%g\" width=\"%g\" "
                  "height=\"%g\">\n",name,svg_info.bounds.x,svg_info.bounds.y,
                  svg_info.bounds.width,svg_info.bounds.height);
                (void) WriteBlobString(image,message);
                break;
              }
            if (LocaleCompare("symbol",token) == 0)
              {
                (void) WriteBlobString(image,"<symbol>\n");
                break;
              }
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'r':
      case 'R':
      {
        if (LocaleCompare("rectangle",keyword) == 0)
          {
            primitive_type=RectanglePrimitive;
            break;
          }
        if (LocaleCompare("roundRectangle",keyword) == 0)
          {
            primitive_type=RoundRectanglePrimitive;
            break;
          }
        if (LocaleCompare("rotate",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,"rotate(%s) ",
              token);
            (void) WriteBlobString(image,message);
            break;
          }
        status=MagickFalse;
        break;
      }
      case '