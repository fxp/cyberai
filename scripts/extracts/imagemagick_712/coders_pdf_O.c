// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 15/30



      /*
        Predict page object id's.
      */
      kid_image=image;
      for ( ; GetNextImageInList(kid_image) != (Image *) NULL; count+=ObjectsPerImage)
      {
        page_count++;
        icc_profile=GetCompatibleColorProfile(kid_image);
        if (icc_profile != (StringInfo *) NULL)
          count+=2;
        (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 R ",(double)
          count);
        (void) WriteBlobString(image,buffer);
        kid_image=GetNextImageInList(kid_image);
      }
      xref=(MagickOffsetType *) ResizeQuantumMemory(xref,(size_t) count+2048UL,
        sizeof(*xref));
      if (xref == (MagickOffsetType *) NULL)
        ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    }
  (void) WriteBlobString(image,"]\n");
  (void) FormatLocaleString(buffer,MagickPathExtent,"/Count %.20g\n",(double)
    page_count);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,">>\n");
  (void) WriteBlobString(image,"endobj\n");
  scene=0;
  number_scenes=GetImageListLength(image);
  do
  {
    Image
      *tile_image;

    MagickBooleanType
      thumbnail;