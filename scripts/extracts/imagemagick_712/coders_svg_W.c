// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 23/39



          if (svg_info->url == (char*) NULL)
            {
              image_info=DestroyImageInfo(image_info);
              (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
              break;
            }
          GetMagickThreadFilename(svg_info->url,thread_filename);
          if (GetValueFromSplayTree(svg_tree,thread_filename) != (const char *) NULL)
            {
              image_info=DestroyImageInfo(image_info);
              (void) ThrowMagickException(svg_info->exception,GetMagickModule(),
                DrawError,"VectorGraphicsNestedTooDeeply","`%s'",svg_info->url);
              break;
            }
          (void) AddValueToSplayTree(svg_tree,ConstantString(thread_filename),
            (void *) 1);
          (void) CopyMagickString(image_info->filename,svg_info->url,
            MagickPathExtent);
          image=ReadImage(image_info,svg_info->exception);
          image_info=DestroyImageInfo(image_info);
          if (image != (Image *) NULL)
            image=DestroyImage(image);
          (void) DeleteNodeFromSplayTree(svg_tree,thread_filename);
          (void) FormatLocaleFile(svg_info->file,
            "image Over %g,%g %g,%g \"%s\"\n",svg_info->bounds.x,
            svg_info->bounds.y,svg_info->bounds.width,svg_info->bounds.height,
            svg_info->url);
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      break;
    }
    case 'L':
    case 'l':
    {
      if (LocaleCompare((const char *) name,"line") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"line %g,%g %g,%g\n",
            svg_info->segment.x1,svg_info->segment.y1,svg_info->segment.x2,
            svg_info->segment.y2);
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      if (LocaleCompare((const char *) name,"linearGradient") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"pop gradient\n");
          break;
        }
      break;
    }
    case 'M':
    case 'm':
    {
      if (LocaleCompare((const char *) name,"mask") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"pop mask\n");
          break;
        }
      break;
    }
    case 'P':
    case 'p':
    {
      if (LocaleCompare((const char *) name,"pattern") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"pop pattern\n");
          break;
        }
      if (LocaleCompare((const char *) name,"path") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"path \"%s\"\n",
            svg_info->vertices);
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      if (LocaleCompare((const char *) name,"polygon") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"polygon %s\n",
            svg_info->vertices);
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      if (LocaleCompare((const char *) name,"polyline") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"polyline %s\n",
            svg_info->vertices);
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      break;
    }
    case 'R':
    case 'r':
    {
      if (LocaleCompare((const char *) name,"radialGradient") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"pop gradient\n");
          break;
        }
      if (LocaleCompare((const char *) name,"rect") == 0)
        {
          if ((svg_info->radius.x == 0.0) && (svg_info->radius.y == 0.0))
            {
              if ((fabs(svg_info->bounds.width-1.0) < MagickEpsilon) &&
                  (fabs(svg_info->bounds.height-1.0) < MagickEpsilon))
                (void) FormatLocaleFile(svg_info->file,"point %g,%g\n",
                  svg_info->bounds.x,svg_info->bounds.y);
              else
                (void) FormatLocaleFile(svg_info->file,
                  "rectangle %g,%g %g,%g\n",svg_info->bounds.x,
                  svg_info->bounds.y,svg_info->bounds.x+svg_info->bounds.width,
                  svg_info->bounds.y+svg_info->bounds.height);
              (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
              break;
            }
          if (svg_info->radius.x == 0.0)
            svg_info->radius.x=svg_info->radius.y;
          if (svg_info->radius.y == 0.0)
            svg_info->radius.y=svg_info->radius.x;
          (void) FormatLocaleFile(svg_info->file,
            "roundRectangle %g,%g %g,%g %g,%g\n",
            svg_info->bounds.x,svg_info->bounds.y,svg_info->bounds.x+
            svg_info->bounds.width,svg_info->bounds.y+svg_info->bounds.height,
            svg_info->radius.x,svg_info->radius.y);
          svg_info->radius.x=0.0;
          svg_info->radius.y=0.0;
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      break;
    }
    case 'S':
    ca