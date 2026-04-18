// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 36/39



                p=q;
                (void) GetNextToken(p,&p,extent,token);
                for (k=0; IsPoint(token); k++)
                  (void) GetNextToken(p,&p,extent,token);
                (void) WriteBlobString(image,"stroke-dasharray:");
                for (j=0; j < k; j++)
                {
                  (void) GetNextToken(q,&q,extent,token);
                  (void) FormatLocaleString(message,MagickPathExtent,"%s ",
                    token);
                  (void) WriteBlobString(image,message);
                }
                (void) WriteBlobString(image,";");
                break;
              }
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,
              "stroke-dasharray:%s;",token);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("stroke-dashoffset",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,
              "stroke-dashoffset:%s;",token);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("stroke-linecap",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,
              "stroke-linecap:%s;",token);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("stroke-linejoin",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,
              "stroke-linejoin:%s;",token);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("stroke-miterlimit",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,
              "stroke-miterlimit:%s;",token);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("stroke-opacity",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,
              "stroke-opacity:%s;",token);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("stroke-width",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,
              "stroke-width:%s;",token);
            (void) WriteBlobString(image,message);
            continue;
          }
        status=MagickFalse;
        break;
      }
      case 't':
      case 'T':
      {
        if (LocaleCompare("text",keyword) == 0)
          {
            primitive_type=TextPrimitive;
            break;
          }
        if (LocaleCompare("text-antialias",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,
              "text-antialias:%s;",token);
            (void) WriteBlobString(image,message);
            break;
          }
        if (LocaleCompare("tspan",keyword) == 0)
          {
            primitive_type=TextPrimitive;
            break;
          }
        if (LocaleCompare("translate",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            affine.tx=StringToDouble(token,&next_token);
            (void) GetNextToken(q,&q,extent,token);
            if (*token == ',')
              (void) GetNextToken(q,&q,extent,token);
            affine.ty=StringToDouble(token,&next_token);
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'v':
      case 'V':
      {
        if (LocaleCompare("viewbox",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            if (*token == ',')
              (void) GetNextToken(q,&q,extent,token);
            (void) GetNextToken(q,&q,extent,token);
            if (*token == ',')
              (void) GetNextToken(q,&q,extent,token);
            (void) GetNextToken(q,&q,extent,token);
            if (*token == ',')
              (void) GetNextToken(q,&q,extent,token);
            (void) GetNextToken(q,&q,extent,token);
            break;
          }
        status=MagickFalse;
        break;
      }
      default:
      {
        status=MagickFalse;
        break;
      }
    }
    if (status == MagickFalse)
      break;
    if (primitive_type == UndefinedPrimitive)
      continue;
    /*
      Parse the primitive attributes.
    */
    i=0;
    j=0;
    for (x=0; *q != '\0'; x++)
    {
      /*
        Define points.
      */
      if (IsPoint(q) == MagickFalse)
        break;
      (void) GetNextToken(q,&q,extent,token);
      point.x=StringToDouble(token,&next_toke