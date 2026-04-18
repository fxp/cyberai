// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 30/39



    delegate_info=GetDelegateInfo((char *) NULL,"TRACE",exception);
    if (delegate_info != (DelegateInfo *) NULL)
      {
        /*
          Trace SVG with tracing delegate.
        */
        image_info=AcquireImageInfo();
        (void) CopyMagickString(image_info->magick,"TRACE",MagickPathExtent);
        (void) FormatLocaleString(filename,MagickPathExtent,"trace:%s",
          image_info->filename);
        (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
        status=WriteImage(image_info,image,exception);
        image_info=DestroyImageInfo(image_info);
        return(status);
      }
    (void) WriteBlobString(image,
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    (void) WriteBlobString(image,
      "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"");
    (void) WriteBlobString(image,
      " \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
    (void) FormatLocaleString(message,MagickPathExtent,
      "<svg version=\"1.1\" id=\"Layer_1\" "
      "xmlns=\"http://www.w3.org/2000/svg\" "
      "xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\" "
      "width=\"%.20gpx\" height=\"%.20gpx\" viewBox=\"0 0 %.20g %.20g\" "
      "enable-background=\"new 0 0 %.20g %.20g\" xml:space=\"preserve\">",
      (double) image->columns,(double) image->rows,
      (double) image->columns,(double) image->rows,
      (double) image->columns,(double) image->rows);
    (void) WriteBlobString(image,message);
    clone_image=CloneImage(image,0,0,MagickTrue,exception);
    if (clone_image == (Image *) NULL)
      return(MagickFalse);
    image_info=AcquireImageInfo();
    (void) CopyMagickString(image_info->magick,"PNG",MagickPathExtent);
    blob_length=2048;
    blob=(unsigned char *) ImageToBlob(image_info,clone_image,&blob_length,
      exception);
    clone_image=DestroyImage(clone_image);
    image_info=DestroyImageInfo(image_info);
    if (blob == (unsigned char *) NULL)
      return(MagickFalse);
    encode_length=0;
    base64=Base64Encode(blob,blob_length,&encode_length);
    blob=(unsigned char *) RelinquishMagickMemory(blob);
    (void) FormatLocaleString(message,MagickPathExtent,
      "  <image id=\"image%.20g\" width=\"%.20g\" height=\"%.20g\" "
      "x=\"%.20g\" y=\"%.20g\"\n    xlink:href=\"data:image/png;base64,",
      (double) image->scene,(double) image->columns,(double) image->rows,
      (double) image->page.x,(double) image->page.y);
    (void) WriteBlobString(image,message);
    p=base64;
    for (i=(ssize_t) encode_length; i > 0; i-=76)
    {
      (void) FormatLocaleString(message,MagickPathExtent,"%.76s",p);
      (void) WriteBlobString(image,message);
      p+=(ptrdiff_t) 76;
      if (i > 76)
        (void) WriteBlobString(image,"\n");
    }
    base64=DestroyString(base64);
    (void) WriteBlobString(image,"\" />\n");
    (void) WriteBlobString(image,"</svg>\n");
  }
#endif
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  return(status);
}

static MagickBooleanType WriteSVGImage(const ImageInfo *image_info,Image *image,
  ExceptionInfo *exception)
{
#define BezierQuantum  200

  AffineMatrix
    affine;

  char
    keyword[MagickPathExtent],
    message[MagickPathExtent],
    name[MagickPathExtent],
    *next_token,
    *token,
    type[MagickPathExtent];

  const char
    *p,
    *q,
    *value;

  int
    n;

  MagickBooleanType
    active,
    status;

  PointInfo
    point;

  PrimitiveInfo
    *primitive_info;

  PrimitiveType
    primitive_type;

  size_t
    extent,
    length,
    number_points;

  ssize_t
    i,
    j,
    x;

  SVGInfo
    svg_info;