// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 32/94



  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d J N G I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadJNGImage() reads a JPEG Network Graphics (JNG) image file
%  (including the 8-byte signature)  and returns it.  It allocates the memory
%  necessary for the new Image structure and returns a pointer to the new
%  image.
%
%  JNG support written by Glenn Randers-Pehrson, glennrp@image...
%
%  The format of the ReadJNGImage method is:
%
%      Image *ReadJNGImage(const ImageInfo *image_info, ExceptionInfo
%         *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static Image *ReadJNGImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    magic_number[MagickPathExtent];

  Image
    *image;

  MagickBooleanType
    logging = MagickFalse,
    status;

  MngReadInfo
    *mng_info;

  size_t
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
    logging=LogMagickEvent(CoderEvent,GetMagickModule(),"Enter ReadJNGImage()");
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);

  if (status == MagickFalse)
    return(DestroyImageList(image));

  if (LocaleCompare(image_info->magick,"JNG") != 0)
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");

  /* Verify JNG signature.  */

  count=(size_t) ReadBlob(image,8,(unsigned char *) magic_number);

  if ((count < 8) || (memcmp(magic_number,"\213JNG\r\n\032\n",8) != 0))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");

  /*
     Verify that file size large enough to contain a JNG datastream.
  */
  if (GetBlobSize(image) < 147)
    ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");

  mng_info=(MngReadInfo *) AcquireMagickMemory(sizeof(*mng_info));
  if (mng_info == (MngReadInfo *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memset(mng_info,0,sizeof(*mng_info));

  mng_info->image=image;
  image=ReadOneJNGImage(mng_info,image_info,exception);
  mng_info=MngReadInfoFreeStruct(mng_info);

  if (image == (Image *) NULL)
    {
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "exit ReadJNGImage() with error");

      return((Image *) NULL);
    }
  (void) CloseBlob(image);

  if (image->columns == 0 || image->rows == 0)
    {
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "exit ReadJNGImage() with error");

      ThrowReaderException(CorruptImageError,"CorruptImage");
    }

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"exit ReadJNGImage()");

  return(image);
}
#endif

static Image *ReadOneMNGImage(MngReadInfo* mng_info,
  const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    page_geometry[MagickPathExtent];

  Image
    *image;

  MagickBooleanType
    logging;

  volatile int
    first_mng_object,
    object_id,
    term_chunk_found,
    skip_to_iend;

  volatile ssize_t
    image_count=0;

  MagickBooleanType
    status;

  MagickOffsetType
    offset;

  MngBox
    default_fb,
    fb,
    previous_fb;

  PixelInfo
    mng_background_color;

  ssize_t
    i;

  size_t
    count;

  ssize_t
    loop_level;

  unsigned char
    *p;

  volatile short
    skipping_loop;

  unsigned int
    mandatory_back=0;

  volatile unsigned int
    mng_type=0;   /* 0: PNG or JNG; 1: MNG; 2: MNG-LC; 3: MNG-VLC */

  size_t
    default_frame_timeout,
    frame_timeout,
    image_height,
    image_width,
    length;

  /* These delays are all measured in image ticks_per_second,
   * not in MNG ticks_per_second
   */
  volatile size_t
    default_frame_delay,
    final_delay,
    final_image_delay,
    frame_delay,
    insert_layers,
    mng_iterations=1,
    simplicity=0,
    subframe_height=0,
    subframe_width=0;