// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 88/94



  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->debug != MagickFalse)
    logging=LogMagickEvent(CoderEvent,GetMagickModule(),
      "Enter WriteJNGImage()");
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(status);
  if ((image->columns > 65535UL) || (image->rows > 65535UL))
    ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");
  mng_info=(MngWriteInfo *) AcquireMagickMemory(sizeof(*mng_info));
  if (mng_info == (MngWriteInfo *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memset(mng_info,0,sizeof(*mng_info));
  mng_info->image=image;

  (void) WriteBlob(image,8,(const unsigned char *) "\213JNG\r\n\032\n");

  status=WriteOneJNGImage(mng_info,image_info,image,exception);

  mng_info=(MngWriteInfo *) RelinquishMagickMemory(mng_info);

  (void) CloseBlob(image);

  (void) CatchImageException(image);
  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"exit WriteJNGImage()");
  return(status);
}
#endif

static MagickBooleanType WriteMNGImage(const ImageInfo *image_info,Image *image,ExceptionInfo *exception)
{
  Image
    *next_image;

  MagickBooleanType
    status,
    write_jng,
    write_mng;

  volatile MagickBooleanType
    logging = MagickFalse;

  MngWriteInfo
    *mng_info;

  int
    image_count,
    need_iterations,
    need_matte;

  volatile int
    need_local_plte,
    all_images_are_gray,
    use_global_plte;

  unsigned char
    chunk[800];

  volatile size_t
    scene;

  size_t
    final_delay=0,
    number_scenes,
    initial_delay;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->debug != MagickFalse)
    logging=LogMagickEvent(CoderEvent,GetMagickModule(),
      "Enter WriteMNGImage()");
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(status);
  mng_info=(MngWriteInfo *) AcquireMagickMemory(sizeof(*mng_info));
  if (mng_info == (MngWriteInfo *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memset(mng_info,0,sizeof(*mng_info));
  mng_info->image=image;
  write_mng=LocaleCompare(image_info->magick,"MNG") == 0 ?
    MagickTrue : MagickFalse;
  /*
   * See if user has requested a specific PNG subformat to be used
   * for all of the PNGs in the MNG being written, e.g.,
   *
   *    convert *.png png8:animation.mng
   *
   * To do: check -define png:bit_depth and png:color_type as well,
   * or perhaps use mng:bit_depth and mng:color_type instead for
   * global settings.
   */

  mng_info->write_png8=LocaleCompare(image_info->magick,"PNG8") == 0 ?
    MagickTrue : MagickFalse;
  mng_info->write_png24=LocaleCompare(image_info->magick,"PNG24") == 0 ?
    MagickTrue : MagickFalse;
  mng_info->write_png32=LocaleCompare(image_info->magick,"PNG32") == 0 ?
    MagickTrue : MagickFalse;

  write_jng=MagickFalse;
  if (image_info->compression == JPEGCompression)
    write_jng=MagickTrue;

  mng_info->adjoin=(image_info->adjoin != MagickFalse) &&
    (write_mng != MagickFalse) &&
    (GetNextImageInList(image) != (Image *) NULL) ?
    MagickTrue : MagickFalse;

  if (logging != MagickFalse)
    {
      /* Log some info about the input */
      Image
        *p;

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  Checking input image(s)\n"
        "    Image_info depth: %.20g,    Type: %d",
        (double) image_info->depth, image_info->type);

      scene=0;
      for (p=image; p != (Image *) NULL; p=GetNextImageInList(p))
      {

        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
           "    Scene: %.20g\n,   Image depth: %.20g",
           (double) scene++, (double) p->depth);

        if (p->alpha_trait != UndefinedPixelTrait)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "      Matte: True");

        else
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "      Matte: False");

        if (p->storage_class == PseudoClass)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "      Storage class: PseudoClass");

        else
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "      Storage class: DirectClass");