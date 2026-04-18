// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 17/30

,buffer);
    (void) WriteBlobString(image,"<<\n");
    (void) WriteBlobString(image,"/Type /Page\n");
    (void) FormatLocaleString(buffer,MagickPathExtent,"/Parent %.20g 0 R\n",
      (double) pages_id);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"/Resources <<\n");
    labels=(char **) NULL;
    value=GetImageProperty(image,"label",exception);
    if (value != (const char *) NULL)
      labels=StringToList(value);
    if (labels != (char **) NULL)
      {
        (void) FormatLocaleString(buffer,MagickPathExtent,
          "/Font << /F%.20g %.20g 0 R >>\n",(double) image->scene,(double)
          object+4);
        (void) WriteBlobString(image,buffer);
      }
    (void) FormatLocaleString(buffer,MagickPathExtent,
      "/XObject << /Im%.20g %.20g 0 R >>\n",(double) image->scene,(double)
      object+5);
    (void) WriteBlobString(image,buffer);
    (void) FormatLocaleString(buffer,MagickPathExtent,"/ProcSet %.20g 0 R >>\n",
      (double) object+3);
    (void) WriteBlobString(image,buffer);
    (void) FormatLocaleString(buffer,MagickPathExtent,
      "/MediaBox [0 0 %g %g]\n",DefaultResolution*media_info.width*
      (double) MagickSafeReciprocal(resolution.x),(double) (
      DefaultResolution*media_info.height*MagickSafeReciprocal(resolution.y)));
    (void) WriteBlobString(image,buffer);
    (void) FormatLocaleString(buffer,MagickPathExtent,
      "/CropBox [0 0 %g %g]\n",DefaultResolution*media_info.width*(double)
      MagickSafeReciprocal(resolution.x),(double) (DefaultResolution*
      media_info.height*MagickSafeReciprocal(resolution.y)));
    (void) WriteBlobString(image,buffer);
    (void) FormatLocaleString(buffer,MagickPathExtent,"/Contents %.20g 0 R\n",
      (double) object+1);
    (void) WriteBlobString(image,buffer);
    (void) FormatLocaleString(buffer,MagickPathExtent,"/Thumb %.20g 0 R\n",
      (double) object+(icc_profile != (StringInfo *) NULL ? 10 : 8));
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,">>\n");
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Contents object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
      object);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"<<\n");
    (void) FormatLocaleString(buffer,MagickPathExtent,"/Length %.20g 0 R\n",
      (double) object+1);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,">>\n");
    (void) WriteBlobString(image,"stream\n");
    offset=TellBlob(image);
    (void) WriteBlobString(image,"q\n");
    if (labels != (char **) NULL)
      for (i=0; labels[i] != (char *) NULL; i++)
      {
        (void) WriteBlobString(image,"BT\n");
        (void) FormatLocaleString(buffer,MagickPathExtent,"/F%.20g %g Tf\n",
          (double) image->scene,pointsize);
        (void) WriteBlobString(image,buffer);
        (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g %.20g Td\n",
          (double) geometry.x,(double) (geometry.y+(ssize_t) geometry.height+
          i*pointsize+12));
        (void) WriteBlobString(image,buffer);
        (void) FormatLocaleString(buffer,MagickPathExtent,"(%s) Tj\n",
           labels[i]);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,"ET\n");
        labels[i]=DestroyString(labels[i]);
      }
    (void) FormatLocaleString(buffer,MagickPathExtent,
      "%g 0 0 %g %.20g %.20g cm\n",scale.x,scale.y,(double) geometry.x,
      (double) geometry.y);
    (void) WriteBlobString(image,buffer);
    (void) FormatLocaleString(buffer,MagickPathExtent,"/Im%.20g Do\n",(double)
      image->scene);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"Q\n");
    offset=TellBlob(image)-offset;
    (void) WriteBlobString(image,"\nendstream\n");
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Length object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
      object);
    (void) WriteBlobString(image,buffer);
    (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g\n",(double)
      offset);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Procset object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
      object);
    (void) WriteBlobString(image,buffer);
    if ((compression == FaxCompression) || (compression == Group4Compression))
      (void) CopyMagickString(buffer,"[ /PDF /Text /ImageB",MagickPathExtent);
    else
      if ((image->storage_class == DirectClass) || (image->colors > 256))
        (void) CopyMagickString(buffer,"[ /PDF /Text /ImageC",MagickPathExtent);
      else
        (void) CopyMagickString(buffer,"[ /PDF /Text /ImageI",MagickPathExtent);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobStr