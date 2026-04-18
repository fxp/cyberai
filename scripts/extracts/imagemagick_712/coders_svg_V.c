// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 22/39



          if ((svg_info->view_box.width < MagickEpsilon) ||
              (svg_info->view_box.height < MagickEpsilon))
            svg_info->view_box=svg_info->bounds;
          svg_info->width=0;
          if (svg_info->bounds.width >= MagickEpsilon)
            svg_info->width=CastDoubleToSizeT(svg_info->bounds.width+0.5);
          svg_info->height=0;
          if (svg_info->bounds.height >= MagickEpsilon)
            svg_info->height=CastDoubleToSizeT(svg_info->bounds.height+0.5);
          (void) FormatLocaleFile(svg_info->file,"viewbox 0 0 %.20g %.20g\n",
            (double) svg_info->width,(double) svg_info->height);
          sx=MagickSafeReciprocal(svg_info->view_box.width)*svg_info->width;
          sy=MagickSafeReciprocal(svg_info->view_box.height)*svg_info->height;
          tx=svg_info->view_box.x != 0.0 ? (double) -sx*svg_info->view_box.x :
            0.0;
          ty=svg_info->view_box.y != 0.0 ? (double) -sy*svg_info->view_box.y :
            0.0;
          (void) FormatLocaleFile(svg_info->file,"affine %g 0 0 %g %g %g\n",
            sx,sy,tx,ty);
          if ((svg_info->svgDepth == 1) && (*background != '\0'))
            {
              PushGraphicContext(id);
              (void) FormatLocaleFile(svg_info->file,"fill %s\n",background);
              (void) FormatLocaleFile(svg_info->file,
                "rectangle 0,0 %g,%g\n",svg_info->view_box.width,
                svg_info->view_box.height);
              (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
            }
        }
    }
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  )");
  if (units != (char *) NULL)
    units=DestroyString(units);
  if (color != (char *) NULL)
    color=DestroyString(color);
}

static void SVGEndElement(void *context,const xmlChar *name)
{
  SVGInfo
    *svg_info;

  xmlParserCtxtPtr
    parser;

  /*
    Called when the end of an element has been detected.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.endElement(%s)",name);
  parser=(xmlParserCtxtPtr) context;
  svg_info=(SVGInfo *) parser->_private;
  if (strchr((char *) name,':') != (char *) NULL)
    {
      /*
        Skip over namespace.
      */
      for ( ; *name != ':'; name++) ;
      name++;
    }
  switch (*name)
  {
    case 'C':
    case 'c':
    {
      if (LocaleCompare((const char *) name,"circle") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"circle %g,%g %g,%g\n",
            svg_info->element.cx,svg_info->element.cy,svg_info->element.cx,
            svg_info->element.cy+svg_info->element.minor);
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      if (LocaleCompare((const char *) name,"clipPath") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"pop clip-path\n");
          break;
        }
      break;
    }
    case 'D':
    case 'd':
    {
      if (LocaleCompare((const char *) name,"defs") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"pop defs\n");
          break;
        }
      if (LocaleCompare((const char *) name,"desc") == 0)
        {
          if (*svg_info->text == '\0')
            break;
          (void) FormatLocaleFile(svg_info->file,"# %s\n",svg_info->text);
          *svg_info->text='\0';
          break;
        }
      break;
    }
    case 'E':
    case 'e':
    {
      if (LocaleCompare((const char *) name,"ellipse") == 0)
        {
          double
            angle;

          angle=svg_info->element.angle;
          (void) FormatLocaleFile(svg_info->file,"ellipse %g,%g %g,%g 0,360\n",
            svg_info->element.cx,svg_info->element.cy,
            angle == 0.0 ? svg_info->element.major : svg_info->element.minor,
            angle == 0.0 ? svg_info->element.minor : svg_info->element.major);
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      break;
    }
    case 'F':
    case 'f':
    {
      if (LocaleCompare((const char *) name,"foreignObject") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      break;
    }
    case 'G':
    case 'g':
    {
      if (LocaleCompare((const char *) name,"g") == 0)
        {
          (void) FormatLocaleFile(svg_info->file,"pop graphic-context\n");
          break;
        }
      break;
    }
    case 'I':
    case 'i':
    {
      if (LocaleCompare((const char *) name,"image") == 0)
        {
          char
            thread_filename[MagickPathExtent];

          Image
            *image;

          ImageInfo
            *image_info = AcquireImageInfo();