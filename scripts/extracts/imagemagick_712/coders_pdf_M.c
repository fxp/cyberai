// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 13/30



  static const char
    XMPProfile[]=
    {
      "<?xpacket begin=\"%s\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>\n"
      "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\" x:xmptk=\"Adobe XMP Core 4.0-c316 44.253921, Sun Oct 01 2006 17:08:23\">\n"
      "   <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
      "      <rdf:Description rdf:about=\"\"\n"
      "            xmlns:xap=\"http://ns.adobe.com/xap/1.0/\"\n"
      "            xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n"
      "            xmlns:xapMM=\"http://ns.adobe.com/xap/1.0/mm/\"\n"
      "            xmlns:pdf=\"http://ns.adobe.com/pdf/1.3/\"\n"
      "            xmlns:pdfaid=\"http://www.aiim.org/pdfa/ns/id/\">\n"
      "         <xap:CreateDate>%s</xap:CreateDate>\n"
      "         <xap:ModifyDate>%s</xap:ModifyDate>\n"
      "         <xap:MetadataDate>%s</xap:MetadataDate>\n"
      "         <xap:CreatorTool>%s</xap:CreatorTool>\n"
      "         <dc:format>application/pdf</dc:format>\n"
      "         <dc:title>\n"
      "           <rdf:Alt>\n"
      "              <rdf:li xml:lang=\"x-default\">%s</rdf:li>\n"
      "           </rdf:Alt>\n"
      "         </dc:title>\n"
      "         <xapMM:DocumentID>uuid:6ec119d7-7982-4f56-808d-dfe64f5b35cf</xapMM:DocumentID>\n"
      "         <xapMM:InstanceID>uuid:a79b99b4-6235-447f-9f6c-ec18ef7555cb</xapMM:InstanceID>\n"
      "         <pdf:Producer>%s</pdf:Producer>\n"
      "         <pdf:Keywords>%s</pdf:Keywords>\n"
      "         <pdfaid:part>3</pdfaid:part>\n"
      "         <pdfaid:conformance>B</pdfaid:conformance>\n"
      "      </rdf:Description>\n"
      "   </rdf:RDF>\n"
      "</x:xmpmeta>\n"
      "<?xpacket end=\"w\"?>\n"
    },
    XMPProfileMagick[4]= { (char) -17, (char) -69, (char) -65, (char) 0 };

  char
    basename[MagickPathExtent],
    buffer[MagickPathExtent],
    **labels,
    temp[MagickPathExtent];

  CompressionType
    compression;

  const char
    *device,
    *option,
    *value;

  const Quantum
    *p;

  const StringInfo
    *icc_profile;

  double
    pointsize,
    version;

  GeometryInfo
    geometry_info;

  Image
    *next;

  MagickBooleanType
    is_pdfa,
    status;

  MagickOffsetType
    offset,
    scene,
    *xref;

  MagickSizeType
    number_pixels;

  MagickStatusType
    flags;

  PointInfo
    delta,
    resolution,
    scale;

  RectangleInfo
    geometry,
    media_info,
    page_info;

  size_t
    channels,
    info_id,
    length,
    number_scenes,
    object,
    pages_id,
    root_id,
    text_size;

  ssize_t
    count,
    i,
    page_count,
    x,
    y;

  struct tm
    utc_time;

  time_t
    seconds;

  unsigned char
    *pixels,
    *q;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(status);
  /*
    Allocate X ref memory.
  */
  xref=(MagickOffsetType *) AcquireQuantumMemory(2048UL,sizeof(*xref));
  if (xref == (MagickOffsetType *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memset(xref,0,2048UL*sizeof(*xref));
  /*
    Write Info object.
  */
  object=0;
  version=1.3;
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
    if (next->alpha_trait != UndefinedPixelTrait)
      version=1.4;
  if (image_info->compression == JPEG2000Compression)
    version=1.5;
  is_pdfa=LocaleCompare(image_info->magick,"PDFA") == 0 ? MagickTrue :
    MagickFalse;
  if (is_pdfa != MagickFalse)
    version=1.6;
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    icc_profile=GetCompatibleColorProfile(next);
    if (icc_profile != (StringInfo *) NULL)
      {
        (void) SetImageStorageClass(next,DirectClass,exception);
        version=1.7;
      }
    if ((next->colorspace != CMYKColorspace) &&
        (IssRGBCompatibleColorspace(next->colorspace) == MagickFalse))
      (void) TransformImageColorspace(next,sRGBColorspace,exception);
    (void) SetImageCoderGray(next,exception);
  }
  option=GetImageOption(image_info,"pdf:version");
  if (option != (const char *) NULL)
    {
      double
        preferred_version;