// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 31/39



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
  value=GetImageArtifact(image,"SVG");
  if (value != (char *) NULL)
    {
      (void) WriteBlobString(image,value);
      (void) CloseBlob(image);
      return(MagickTrue);
    }
  value=GetImageArtifact(image,"mvg:vector-graphics");
  if (value == (char *) NULL)
    return(TraceSVGImage(image,exception));
  /*
    Write SVG header.
  */
  (void) WriteBlobString(image,
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
  (void) FormatLocaleString(message,MagickPathExtent,
    "<svg width=\"%.20g\" height=\"%.20g\" xmlns=\"http://www.w3.org/2000/svg\">\n",
    (double) image->columns,(double) image->rows);
  (void) WriteBlobString(image,message);
  /*
    Allocate primitive info memory.
  */
  number_points=2047;
  primitive_info=(PrimitiveInfo *) AcquireQuantumMemory(number_points,
    sizeof(*primitive_info));
  if (primitive_info == (PrimitiveInfo *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  GetAffineMatrix(&affine);
  token=AcquireString(value);
  extent=strlen(token)+MagickPathExtent;
  active=MagickFalse;
  n=0;
  status=MagickTrue;
  for (q=(const char *) value; *q != '\0'; )
  {
    /*
      Interpret graphic primitive.
    */
    (void) GetNextToken(q,&q,MagickPathExtent,keyword);
    if (*keyword == '\0')
      break;
    if (*keyword == '#')
      {
        /*
          Comment.
        */
        if (active != MagickFalse)
          {
            AffineToTransform(image,&affine);
            active=MagickFalse;
          }
        (void) WriteBlobString(image,"<desc>");
        (void) WriteBlobString(image,keyword+1);
        for ( ; (*q != '\n') && (*q != '\0'); q++)
          switch (*q)
          {
            case '<': (void) WriteBlobString(image,"&lt;"); break;
            case '>': (void) WriteBlobString(image,"&gt;"); break;
            case '&': (void) WriteBlobString(image,"&amp;"); break;
            default: (void) WriteBlobByte(image,(unsigned char) *q); break;
          }
        (void) WriteBlobString(image,"</desc>\n");
        continue;
      }
    primitive_type=UndefinedPrimitive;
    switch (*keyword)
    {
      case ';':
        break;
      case 'a':
      case 'A':
      {
        if (LocaleCompare("affine",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            affine.sx=StringToDouble(token,&next_token);
            (void) GetNextToken(q,&q,extent,token);
            if (*token == ',')
              (void) GetNextToken(q,&q,extent,token);
            affine.rx=StringToDouble(token,&next_token);
            (void) GetNextToken(q,&q,extent,token);
            if (*token == ',')
              (void) GetNextToken(q,&q,extent,token);
            affine.ry=StringToDouble(token,&next_token);
            (void) GetNextToken(q,&q,extent,token);
            if (*token == ',')
              (void) GetNextToken(q,&q,extent,token);
            affine.sy=StringToDouble(token,&next_token);
            (void) GetNextToken(q,&q,extent,token);
            if (*token == ',')
              (void) GetNextToken(q,&q,extent,token);
            affine.tx=StringToDouble(token,&next_token);
            (void) GetNextToken(q,&q,extent,token);
            if (*token == ',')
              (void) GetNextToken(q,&q,extent,token);
            affine.ty=StringToDouble(token,&next_token);
            break;
          }
        if (LocaleCompare("alpha",keyword) == 0)
          {
            primitive_type=AlphaPrimitive;
            break;
          }
        if (LocaleCompare("angle",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            affine.rx=StringToDouble(token,&next_token);
            affine.ry=StringToDouble(token,&next_token);
            break;
          }
        if (LocaleCompare("arc",keyword) == 0)
          {
            primitive_type=ArcPrimitive;
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'b':
      case 'B':
      {
        if (LocaleCompare("bezier",keyword) == 0)
          {
            primitive_type=BezierPrimitive;
            break;
          }
        status=MagickFalse;
        break;
      }
      case 'c':
      case 'C':
      {
        if (LocaleCompare("clip-path",keyword) == 0)
          {
            (void) GetNextToken(q,&q,extent,token);
            (void) FormatLocaleString(message,MagickPathExtent,
              "clip-path:url(#%s)