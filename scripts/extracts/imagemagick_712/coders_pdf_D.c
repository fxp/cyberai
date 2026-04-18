// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 4/30



        /*
          Note spot names.
        */
        (void) FormatLocaleString(property,MagickPathExtent,
          "pdf:SpotColor-%.20g",(double) spotcolor++);
        i=0;
        SkipMagickByteBuffer(&buffer,strlen(SpotColor)+1);
        for (c=ReadMagickByteBuffer(&buffer); c != EOF; c=ReadMagickByteBuffer(&buffer))
        {
          if ((isspace((int) ((unsigned char) c)) != 0) || (c == '/') || ((i+1) == MagickPathExtent))
            break;
          name[i++]=(char) c;
        }
        if (c == EOF)
          break;
        name[i]='\0';
        value=ConstantString(name);
        (void) SubstituteString(&value,"#20"," ");
        if (*value != '\0')
          (void) SetImageProperty(image,property,value,exception);
        value=DestroyString(value);
        continue;
      }
    if (image_info->page != (char *) NULL)
      continue;
    count=0;
    if (pdf_info->cropbox != MagickFalse)
      {
        if (CompareMagickByteBuffer(&buffer,CropBox,strlen(CropBox)) != MagickFalse)
          {
            /*
              Note region defined by crop box.
            */
            p=GetMagickByteBufferDatum(&buffer);
            count=(ssize_t) MagickSscanf(p,"CropBox [%lf %lf %lf %lf",&bounds.x1,
              &bounds.y1,&bounds.x2,&bounds.y2);
            if (count != 4)
              count=(ssize_t) MagickSscanf(p,"CropBox[%lf %lf %lf %lf",&bounds.x1,
                &bounds.y1,&bounds.x2,&bounds.y2);
          }
      }
    else
      if (pdf_info->trimbox != MagickFalse)
        {
          if (CompareMagickByteBuffer(&buffer,TrimBox,strlen(TrimBox)) != MagickFalse)
            {
              /*
                Note region defined by trim box.
              */
              p=GetMagickByteBufferDatum(&buffer);
              count=(ssize_t) MagickSscanf(p,"TrimBox [%lf %lf %lf %lf",&bounds.x1,
                &bounds.y1,&bounds.x2,&bounds.y2);
              if (count != 4)
                count=(ssize_t) MagickSscanf(p,"TrimBox[%lf %lf %lf %lf",&bounds.x1,
                  &bounds.y1,&bounds.x2,&bounds.y2);
            }
        }
      else
        if (CompareMagickByteBuffer(&buffer,MediaBox,strlen(MediaBox)) != MagickFalse)
          {
            /*
              Note region defined by media box.
            */
            p=GetMagickByteBufferDatum(&buffer);
            count=(ssize_t) MagickSscanf(p,"MediaBox [%lf %lf %lf %lf",&bounds.x1,
              &bounds.y1,&bounds.x2,&bounds.y2);
            if (count != 4)
              count=(ssize_t) MagickSscanf(p,"MediaBox[%lf %lf %lf %lf",&bounds.x1,
                &bounds.y1,&bounds.x2,&bounds.y2);
          }
    if (count != 4)
      continue;
    if ((fabs(bounds.x2-bounds.x1) <= fabs(pdf_info->bounds.x2-pdf_info->bounds.x1)) ||
        (fabs(bounds.y2-bounds.y1) <= fabs(pdf_info->bounds.y2-pdf_info->bounds.y1)))
      continue;
    pdf_info->bounds=bounds;
  }
  if (version[0] != '\0')
    (void) SetImageProperty(image,"pdf:Version",version,exception);
}

static inline void CleanupPDFInfo(PDFInfo *pdf_info)
{
  if (pdf_info->xmp_profile != (StringInfo *) NULL)
    pdf_info->xmp_profile=DestroyStringInfo(pdf_info->xmp_profile);
}

static Image *ReadPDFImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    command[MagickPathExtent],
    *density,
    filename[MagickPathExtent],
    input_filename[MagickPathExtent],
    message[MagickPathExtent],
    *options,
    postscript_filename[MagickPathExtent];

  const char
    *option;

  const DelegateInfo
    *delegate_info;

  GeometryInfo
    geometry_info;

  Image
    *image,
    *next,
    *pdf_image;

  ImageInfo
    *read_info;

  int
    file;

  MagickBooleanType
    fitPage,
    status;

  MagickStatusType
    flags;

  PDFInfo
    pdf_info;

  PointInfo
    delta;

  RectangleInfo
    page;

  ssize_t
    i;

  size_t
    scene;