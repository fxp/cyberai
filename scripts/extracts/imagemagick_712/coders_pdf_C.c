// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 3/30



  (void) memset(&bounds,0,sizeof(bounds));
  (void) memset(pdf_info,0,sizeof(*pdf_info));
  pdf_info->cmyk=image_info->colorspace == CMYKColorspace ? MagickTrue :
    MagickFalse;
  pdf_info->cropbox=IsStringTrue(GetImageOption(image_info,"pdf:use-cropbox"));
  pdf_info->trimbox=IsStringTrue(GetImageOption(image_info,"pdf:use-trimbox"));
  *version='\0';
  spotcolor=0;
  percent_count=0;
  (void) memset(&buffer,0,sizeof(buffer));
  buffer.image=image;
  for (c=ReadMagickByteBuffer(&buffer); c != EOF; c=ReadMagickByteBuffer(&buffer))
  {
    if (c == '%')
      {
        if (*version == '\0')
          {
            i=0;
            for (c=ReadMagickByteBuffer(&buffer); c != EOF; c=ReadMagickByteBuffer(&buffer))
            {
              if ((c == '\r') || (c == '\n') || ((i+1) == MagickPathExtent))
                break;
              version[i++]=(char) c;
            }
            version[i]='\0';
            if (c == EOF)
              break;
          }
        if (++percent_count == 2)
          percent_count=0;
        else
          continue;
      }
    else
      {
        percent_count=0;
        switch(c)
        {
          case '<':
          {
            ReadGhostScriptXMPProfile(&buffer,&pdf_info->xmp_profile,exception);
            continue;
          }
          case '/':
            break;
          default:
            continue;
        }
      }
    if (CompareMagickByteBuffer(&buffer,PDFRotate,strlen(PDFRotate)) != MagickFalse)
      {
        p=GetMagickByteBufferDatum(&buffer);
        (void) MagickSscanf(p,PDFRotate" %lf",&pdf_info->angle);
      }
    if (pdf_info->cmyk == MagickFalse)
      {
        if ((CompareMagickByteBuffer(&buffer,DefaultCMYK,strlen(DefaultCMYK)) != MagickFalse) ||
            (CompareMagickByteBuffer(&buffer,DeviceCMYK,strlen(DeviceCMYK)) != MagickFalse) ||
            (CompareMagickByteBuffer(&buffer,CMYKProcessColor,strlen(CMYKProcessColor)) != MagickFalse))
          {
            pdf_info->cmyk=MagickTrue;
            continue;
          }
      }
    if (CompareMagickByteBuffer(&buffer,SpotColor,strlen(SpotColor)) != MagickFalse)
      {
        char
          name[MagickPathExtent],
          property[MagickPathExtent],
          *value;