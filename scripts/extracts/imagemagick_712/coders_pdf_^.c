// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 30/30

iteBlobString(image,buffer);
  (void) WriteBlobString(image,"0000000000 65535 f \n");
  for (i=0; i < (ssize_t) object; i++)
  {
    (void) FormatLocaleString(buffer,MagickPathExtent,"%010lu 00000 n \n",
      (unsigned long) xref[i]);
    (void) WriteBlobString(image,buffer);
  }
  (void) WriteBlobString(image,"trailer\n");
  (void) WriteBlobString(image,"<<\n");
  (void) FormatLocaleString(buffer,MagickPathExtent,"/Size %.20g\n",(double)
    object+1);
  (void) WriteBlobString(image,buffer);
  (void) FormatLocaleString(buffer,MagickPathExtent,"/Info %.20g 0 R\n",(double)
    info_id);
  (void) WriteBlobString(image,buffer);
  (void) FormatLocaleString(buffer,MagickPathExtent,"/Root %.20g 0 R\n",(double)
    root_id);
  (void) WriteBlobString(image,buffer);
  option=GetImageOption(image_info,"pdf:no-identifier");
  if ((IsStringFalse(option) != MagickFalse) || (is_pdfa != MagickFalse))
    {
      (void) SignatureImage(image,exception);
      (void) FormatLocaleString(buffer,MagickPathExtent,"/ID [<%s> <%s>]\n",
        GetImageProperty(image,"signature",exception),
        GetImageProperty(image,"signature",exception));
      (void) WriteBlobString(image,buffer);
    }
  (void) WriteBlobString(image,">>\n");
  (void) WriteBlobString(image,"startxref\n");
  (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g\n",(double) offset);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"%%EOF\n");
  xref=(MagickOffsetType *) RelinquishMagickMemory(xref);
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  return(status);
}
