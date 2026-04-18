// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 14/18



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
    Allocate colormap.
  */
  global_colormap=(unsigned char *) AcquireQuantumMemory(768UL,
    sizeof(*global_colormap));
  colormap=(unsigned char *) AcquireQuantumMemory(768UL,sizeof(*colormap));
  if ((global_colormap == (unsigned char *) NULL) ||
      (colormap == (unsigned char *) NULL))
    {
      if (global_colormap != (unsigned char *) NULL)
        global_colormap=(unsigned char *) RelinquishMagickMemory(
          global_colormap);
      if (colormap != (unsigned char *) NULL)
        colormap=(unsigned char *) RelinquishMagickMemory(colormap);
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    }
  for (i=0; i < 768; i++)
    colormap[i]=(unsigned char) 0;
  /*
    Write GIF header.
  */
  write_info=CloneImageInfo(image_info);
  if (LocaleCompare(write_info->magick,"GIF87") != 0)
    (void) WriteBlob(image,6,(unsigned char *) "GIF89a");
  else
    {
      (void) WriteBlob(image,6,(unsigned char *) "GIF87a");
      write_info->adjoin=MagickFalse;
    }
  /*
    Determine image bounding box.
  */
  page.width=image->columns;
  if (image->page.width > page.width)
    page.width=image->page.width;
  page.height=image->rows;
  if (image->page.height > page.height)
    page.height=image->page.height;
  page.x=image->page.x;
  page.y=image->page.y;
  (void) WriteBlobLSBShort(image,(unsigned short) page.width);
  (void) WriteBlobLSBShort(image,(unsigned short) page.height);
  /*
    Write images to file.
  */
  scene=0;
  one=1;
  number_scenes=GetImageListLength(image);
  do
  {
    if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
      (void) TransformImageColorspace(image,sRGBColorspace,exception);
    opacity=(-1);
    if (IsImageOpaque(image,exception) != MagickFalse)
      {
        if ((image->storage_class == DirectClass) || (image->colors > 256))
          (void) SetImageType(image,PaletteType,exception);
      }
    else
      {
        double
          alpha,
          beta;