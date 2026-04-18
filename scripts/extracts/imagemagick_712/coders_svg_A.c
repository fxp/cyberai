// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 33/39

   "text-anchor %s ",token);
            (void) WriteBlobString(image,message);
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'i':
      case 'I':
      {
        if (LocaleCompare("image",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            primitive_type=ImagePrimitive;
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'k':
      case 'K':
      {
        if (LocaleCompare("kerning",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,"kerning:%s;",
              token);
            (void) WriteBlobString(image,message);
          }
        break;
      }
      case 'l':
      case 'L':
      {
        if (LocaleCompare("letter-spacing",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,
              "letter-spacing:%s;",token);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("line",keyword) == 0)
          {
            primitive_type=LinePrimitive;
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'o':
      case 'O':
      {
        if (LocaleCompare("opacity",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,"opacity %s ",
              token);
            (void) WriteBlobString(image,message);
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'p':
      case 'P':
      {
        if (LocaleCompare("path",keyword) == 0)
          {
            primitive_type=PathPrimitive;
            break;
          }
        if (LocaleCompare("point",keyword) == 0)
          {
            primitive_type=PointPrimitive;
            break;
          }
        if (LocaleCompare("polyline",keyword) == 0)
          {
            primitive_type=PolylinePrimitive;
            break;
          }
        if (LocaleCompare("polygon",keyword) == 0)
          {
            primitive_type=PolygonPrimitive;
            break;
          }
        if (LocaleCompare("pop",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            if (LocaleCompare("clip-path",token) == 0)
              {
                (void) WriteBlobString(image,"</clipPath>\n");
                break;
              }
            if (LocaleCompare("defs",token) == 0)
              {
                (void) WriteBlobString(image,"</defs>\n");
                break;
              }
            if (LocaleCompare("gradient",token) == 0)
              {
                (void) FormatLocaleString(message,MagickPathExtent,
                  "</%sGradient>\n",type);
                (void) WriteBlobString(image,message);
                break;
              }
            if (LocaleCompare("graphic-context",token) == 0)
              {
                n--;
                if (n < 0)
                  ThrowWriterException(DrawError,
                    "UnbalancedGraphicContextPushPop");
                (void) WriteBlobString(image,"</g>\n");
              }
            if (LocaleCompare("pattern",token) == 0)
              {
                (void) WriteBlobString(image,"</pattern>\n");
                break;
              }
            if (LocaleCompare("symbol",token) == 0)
              {
                (void) WriteBlobString(image,"</symbol>\n");
                break;
              }
            if ((LocaleCompare("defs",token) == 0) ||
                (LocaleCompare("symbol",token) == 0))
              (void) WriteBlobString(image,"</g>\n");
            break;
          }
        if (LocaleCompare("push",keyword) == 0)
          {
            *name='\0';
            (void) GetNextToken(q,&q,extent,token);
            if (*q == '"')
              (void) GetNextToken(q,&q,MagickPathExtent,name);
            if (LocaleCompare("clip-path",token) == 0)
              {
                (void) GetNextToken(q,&q,extent,token);
                (void) FormatLocaleString(message,MagickPathExtent,
                  "<clipPath id=\"%s\">\n",token);
                (void) WriteBlobString(image,message);
                break;
              }
            if (LocaleCompare("defs",token) == 0)
              {
                (void) WriteBlobString(image,"<defs>\n");
                break;
              }
            if (LocaleCompare("gradient",token) == 0)
              {
                (void) GetNextToken(q,&q,extent,token);
                (void) CopyMagickString(name,token,MagickPathExtent);
                (void) GetNextToken(q,&q,extent,token);
                (void) CopyMagickString(type,token,MagickPathExtent);
                (void) GetNextToken(q,&q,extent,token);
                svg_info.segment.x1=StringTo