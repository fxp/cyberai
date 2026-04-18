// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 21/39

     }
          break;
        }
        case 'V':
        case 'v':
        {
          if (LocaleCompare(keyword,"verts") == 0)
            {
              (void) CloneString(&svg_info->vertices,value);
              break;
            }
          if (LocaleCompare(keyword,"viewBox") == 0)
            {
              p=value;
              (void) GetNextToken(p,&p,MagickPathExtent,token);
              svg_info->view_box.x=StringToDouble(token,&next_token);
              (void) GetNextToken(p,&p,MagickPathExtent,token);
              if (*token == ',')
                (void) GetNextToken(p,&p,MagickPathExtent,token);
              svg_info->view_box.y=StringToDouble(token,&next_token);
              (void) GetNextToken(p,&p,MagickPathExtent,token);
              if (*token == ',')
                (void) GetNextToken(p,&p,MagickPathExtent,token);
              svg_info->view_box.width=StringToDouble(token,
                (char **) NULL);
              if (svg_info->bounds.width < MagickEpsilon)
                svg_info->bounds.width=svg_info->view_box.width;
              (void) GetNextToken(p,&p,MagickPathExtent,token);
              if (*token == ',')
                (void) GetNextToken(p,&p,MagickPathExtent,token);
              svg_info->view_box.height=StringToDouble(token,
                (char **) NULL);
              if (svg_info->bounds.height == 0)
                svg_info->bounds.height=svg_info->view_box.height;
              break;
            }
          break;
        }
        case 'W':
        case 'w':
        {
          if (LocaleCompare(keyword,"width") == 0)
            {
              svg_info->bounds.width=
                GetUserSpaceCoordinateValue(svg_info,1,value);
              break;
            }
          break;
        }
        case 'X':
        case 'x':
        {
          if (LocaleCompare(keyword,"x") == 0)
            {
              svg_info->bounds.x=GetUserSpaceCoordinateValue(svg_info,1,value);
              break;
            }
          if (LocaleCompare(keyword,"xlink:href") == 0)
            {
              (void) CloneString(&svg_info->url,value);
              break;
            }
          if (LocaleCompare(keyword,"x1") == 0)
            {
              svg_info->segment.x1=
                GetUserSpaceCoordinateValue(svg_info,1,value);
              break;
            }
          if (LocaleCompare(keyword,"x2") == 0)
            {
              svg_info->segment.x2=
                GetUserSpaceCoordinateValue(svg_info,1,value);
              break;
            }
          break;
        }
        case 'Y':
        case 'y':
        {
          if (LocaleCompare(keyword,"y") == 0)
            {
              svg_info->bounds.y=GetUserSpaceCoordinateValue(svg_info,-1,value);
              break;
            }
          if (LocaleCompare(keyword,"y1") == 0)
            {
              svg_info->segment.y1=
                GetUserSpaceCoordinateValue(svg_info,-1,value);
              break;
            }
          if (LocaleCompare(keyword,"y2") == 0)
            {
              svg_info->segment.y2=
                GetUserSpaceCoordinateValue(svg_info,-1,value);
              break;
            }
          break;
        }
        default:
          break;
      }
      value=DestroyString(value);
    }
  if (LocaleCompare((const char *) name,"svg") == 0)
    {
      if (parser->encoding != (const xmlChar *) NULL)
        (void) FormatLocaleFile(svg_info->file,"encoding \"%s\"\n",
          (const char *) parser->encoding);
      if (attributes != (const xmlChar **) NULL)
        {
          double
            sx,
            sy,
            tx,
            ty;