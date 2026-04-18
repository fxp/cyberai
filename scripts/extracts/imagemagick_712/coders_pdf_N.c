// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 14/30



      preferred_version=StringToDouble(option,(char**) NULL);
      version=MagickMax(version,MagickMin(1.7,preferred_version));
    }
  (void) FormatLocaleString(buffer,MagickPathExtent,"%%PDF-%.2g\n",version);
  (void) WriteBlobString(image,buffer);
  if (is_pdfa != MagickFalse)
    {
      (void) WriteBlobByte(image,'%');
      (void) WriteBlobByte(image,0xe2);
      (void) WriteBlobByte(image,0xe3);
      (void) WriteBlobByte(image,0xcf);
      (void) WriteBlobByte(image,0xd3);
      (void) WriteBlobByte(image,'\n');
    }
  /*
    Write Catalog object.
  */
  xref[object++]=TellBlob(image);
  root_id=object;
  (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
    object);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"<<\n");
  if (is_pdfa == MagickFalse)
    (void) FormatLocaleString(buffer,MagickPathExtent,"/Pages %.20g 0 R\n",
      (double) object+1);
  else
    {
      (void) FormatLocaleString(buffer,MagickPathExtent,"/Metadata %.20g 0 R\n",
        (double) object+1);
      (void) WriteBlobString(image,buffer);
      (void) FormatLocaleString(buffer,MagickPathExtent,"/Pages %.20g 0 R\n",
        (double) object+2);
    }
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"/Type /Catalog");
  option=GetImageOption(image_info,"pdf:page-direction");
  if ((option != (const char *) NULL) &&
      (LocaleCompare(option,"right-to-left") == 0))
    (void) WriteBlobString(image,"/ViewerPreferences<</PageDirection/R2L>>\n");
  (void) WriteBlobString(image,"\n");
  (void) WriteBlobString(image,">>\n");
  (void) WriteBlobString(image,"endobj\n");
  GetPathComponent(image->filename,BasePath,basename);
  if (is_pdfa != MagickFalse)
    {
      char
        create_date[MagickTimeExtent],
        *creator,
        *keywords,
        modify_date[MagickTimeExtent],
        *producer,
        timestamp[MagickTimeExtent],
        *title;

      /*
        Write XMP object.
      */
      xref[object++]=TellBlob(image);
      (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
        object);
      (void) WriteBlobString(image,buffer);
      (void) WriteBlobString(image,"<<\n");
      (void) WriteBlobString(image,"/Subtype /XML\n");
      (void) FormatMagickTime(GetPdfCreationDate(image_info,image),
        sizeof(create_date),create_date);
      (void) FormatMagickTime(GetPdfModDate(image_info,image),
        sizeof(modify_date),modify_date);
      (void) FormatMagickTime(GetMagickTime(),sizeof(timestamp),timestamp);
      creator=SubstituteXMLEntities(GetPDFCreator(image_info),MagickFalse);
      title=SubstituteXMLEntities(GetPDFTitle(image_info,basename),MagickFalse);
      producer=SubstituteXMLEntities(GetPDFProducer(image_info),MagickFalse);
      keywords=SubstituteXMLEntities(GetPDFKeywords(image_info),MagickFalse);
      i=FormatLocaleString(temp,MagickPathExtent,XMPProfile,XMPProfileMagick,
        create_date,modify_date,timestamp,creator,title,producer,keywords);
      producer=DestroyString(producer);
      title=DestroyString(title);
      creator=DestroyString(creator);
      keywords=DestroyString(keywords);
      (void) FormatLocaleString(buffer,MagickPathExtent,"/Length %.20g\n",
        (double) i);
      (void) WriteBlobString(image,buffer);
      (void) WriteBlobString(image,"/Type /Metadata\n");
      (void) WriteBlobString(image,">>\nstream\n");
      (void) WriteBlobString(image,temp);
      (void) WriteBlobString(image,"\nendstream\n");
      (void) WriteBlobString(image,"endobj\n");
    }
  /*
    Write Pages object.
  */
  xref[object++]=TellBlob(image);
  pages_id=object;
  (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
    object);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"<<\n");
  (void) WriteBlobString(image,"/Type /Pages\n");
  (void) FormatLocaleString(buffer,MagickPathExtent,"/Kids [ %.20g 0 R ",
    (double) object+1);
  (void) WriteBlobString(image,buffer);
  count=(ssize_t) (pages_id+ObjectsPerImage+1);
  page_count=1;
  if (image_info->adjoin != MagickFalse)
    {
      Image
        *kid_image;