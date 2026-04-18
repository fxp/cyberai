// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 49/94



  while (GetNextImageInList(image) != (Image *) NULL)
      image=GetNextImageInList(image);

  image->dispose=BackgroundDispose;

  if (logging != MagickFalse)
    {
      int
        scene;

      scene=0;
      image=GetFirstImageInList(image);

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  After coalesce:");

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    scene 0 delay=%.20g dispose=%.20g",(double) image->delay,
        (double) image->dispose);

      while (GetNextImageInList(image) != (Image *) NULL)
      {
        image=GetNextImageInList(image);

        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    scene %.20g delay=%.20g dispose=%.20g",(double) scene++,
          (double) image->delay,(double) image->dispose);
      }
   }

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  exit ReadOneMNGImage();");

  return(image);
}

static Image *ReadMNGImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image
    *image;

  MagickBooleanType
    logging = MagickFalse,
    status;

  MngReadInfo
    *mng_info;

  /* Open image file.  */

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
       image_info->filename);
  image=AcquireImage(image_info,exception);
  if (image->debug != MagickFalse)
    logging=LogMagickEvent(CoderEvent,GetMagickModule(),"Enter ReadMNGImage()");
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);

  if (status == MagickFalse)
    return(DestroyImageList(image));

  mng_info=(MngReadInfo *) AcquireMagickMemory(sizeof(*mng_info));

  if (mng_info == (MngReadInfo *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");

  (void) memset(mng_info,0,sizeof(*mng_info));
  mng_info->image=image;
  image=ReadOneMNGImage(mng_info,image_info,exception);
  mng_info=MngReadInfoFreeStruct(mng_info);

  if (image == (Image *) NULL)
    {
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "exit ReadMNGImage() with error");

      return((Image *) NULL);
    }
  (void) CloseBlob(image);

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"exit ReadMNGImage()");

  return(GetFirstImageInList(image));
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P N G I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterPNGImage() adds properties for the PNG image format to
%  the list of supported formats.  The properties include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterPNGImage method is:
%
%      size_t RegisterPNGImage(void)
%
*/
ModuleExport size_t RegisterPNGImage(void)
{
  char
    version[MagickPathExtent];

  MagickInfo
    *entry;

  static const char
    PNGNote[] =
    {
      "See http://www.libpng.org/ for details about the PNG format."
    },

    JNGNote[] =
    {
      "See http://www.libpng.org/pub/mng/ for details about the JNG\n"
      "format."
    },

    MNGNote[] =
    {
      "See http://www.libpng.org/pub/mng/ for details about the MNG\n"
      "format."
    };

  *version='\0';

#if defined(PNG_LIBPNG_VER_STRING)
  (void) ConcatenateMagickString(version,"libpng ",MagickPathExtent);
  (void) ConcatenateMagickString(version,PNG_LIBPNG_VER_STRING,
   MagickPathExtent);

  if (LocaleCompare(PNG_LIBPNG_VER_STRING,png_get_header_ver(NULL)) != 0)
    {
      (void) ConcatenateMagickString(version,",",MagickPathExtent);
      (void) ConcatenateMagickString(version,png_get_libpng_ver(NULL),
            MagickPathExtent);
    }
#endif

  entry=AcquireMagickInfo("PNG","MNG","Multiple-image Network Graphics");
  entry->flags|=CoderDecoderSeekableStreamFlag;

#if defined(MAGICKCORE_PNG_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadMNGImage;
  entry->encoder=(EncodeImageHandler *) WriteMNGImage;
#endif