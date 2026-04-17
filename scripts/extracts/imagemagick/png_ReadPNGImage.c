/* ===== EXTRACT: png.c ===== */
/* Function: ReadPNGImage */
/* Lines: 3911–4030 */

static Image *ReadPNGImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  Image
    *image;

  MagickBooleanType
    logging = MagickFalse,
    status;

  MngReadInfo
    *mng_info;

  char
    magic_number[MagickPathExtent];

  ssize_t
    count;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  image=AcquireImage(image_info,exception);
  if (image->debug != MagickFalse)
    logging=LogMagickEvent(CoderEvent,GetMagickModule(),"Enter ReadPNGImage()");
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);

  if (status == MagickFalse)
    return(DestroyImageList(image));

  /*
    Verify PNG signature.
  */
  count=ReadBlob(image,8,(unsigned char *) magic_number);

  if ((count < 8) || (memcmp(magic_number,"\211PNG\r\n\032\n",8) != 0))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");

  /*
     Verify that file size large enough to contain a PNG datastream.
  */
  if (GetBlobSize(image) < 61)
    ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");

  mng_info=(MngReadInfo *) AcquireMagickMemory(sizeof(*mng_info));
  if (mng_info == (MngReadInfo *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memset(mng_info,0,sizeof(*mng_info));
  mng_info->image=image;

  image=ReadOnePNGImage(mng_info,image_info,exception);
  mng_info=MngReadInfoFreeStruct(mng_info);

  if (image == (Image *) NULL)
    {
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "exit ReadPNGImage() with error");

      return((Image *) NULL);
    }

  (void) CloseBlob(image);

  if ((image->columns == 0) || (image->rows == 0))
    {
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "exit ReadPNGImage() with error.");

      ThrowReaderException(CorruptImageError,"CorruptImage");
    }

  if ((IssRGBColorspace(image->colorspace) != MagickFalse) &&
      (image->gamma > .75) &&
           !(image->chromaticity.red_primary.x>0.6399 &&
           image->chromaticity.red_primary.x<0.6401 &&
           image->chromaticity.red_primary.y>0.3299 &&
           image->chromaticity.red_primary.y<0.3301 &&
           image->chromaticity.green_primary.x>0.2999 &&
           image->chromaticity.green_primary.x<0.3001 &&
           image->chromaticity.green_primary.y>0.5999 &&
           image->chromaticity.green_primary.y<0.6001 &&
           image->chromaticity.blue_primary.x>0.1499 &&
           image->chromaticity.blue_primary.x<0.1501 &&
           image->chromaticity.blue_primary.y>0.0599 &&
           image->chromaticity.blue_primary.y<0.0601 &&
           image->chromaticity.white_point.x>0.3126 &&
           image->chromaticity.white_point.x<0.3128 &&
           image->chromaticity.white_point.y>0.3289 &&
           image->chromaticity.white_point.y<0.3291))
    {
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "SetImageColorspace to RGBColorspace");
      SetImageColorspace(image,RGBColorspace,exception);
    }

  if (logging != MagickFalse)
    {
       (void) LogMagickEvent(CoderEvent,GetMagickModule(),
           "  page.w: %.20g, page.h: %.20g,page.x: %.20g, page.y: %.20g.",
               (double) image->page.width,(double) image->page.height,
               (double) image->page.x,(double) image->page.y);
       (void) LogMagickEvent(CoderEvent,GetMagickModule(),
           "  image->colorspace: %d", (int) image->colorspace);
    }

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"exit ReadPNGImage()");

  return(image);
}
