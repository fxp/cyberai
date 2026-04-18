// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 8/17



static void ReadHEICDepthImage(const ImageInfo *image_info,Image *image,
  struct heif_context *heif_context,struct heif_image_handle *image_handle,
  ExceptionInfo *exception)
{
  const char
    *option;

  heif_item_id
    depth_id;

  int
    number_images;

  struct heif_error
    error;

  struct heif_image_handle
    *depth_handle;

  /*
    Read HEIF depth image.
  */
  option=GetImageOption(image_info,"heic:depth-image");
  if (IsStringTrue(option) == MagickFalse)
    return;
  if (heif_image_handle_has_depth_image(image_handle) == 0)
    return;
  number_images=heif_image_handle_get_list_of_depth_image_IDs(image_handle,
    &depth_id,1);
  if (number_images != 1)
    return;
  error=heif_image_handle_get_depth_image_handle(image_handle,depth_id,
    &depth_handle);
  if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
    return;
  AcquireNextImage(image_info,image,exception);
  if (GetNextImageInList(image) != (Image *) NULL)
    {
      image=SyncNextImageInList(image);
      (void) ReadHEICImageHandle(image_info,image,heif_context,depth_handle,exception);
    }
  heif_image_handle_release(depth_handle);
}

static Image *ReadHEICImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  enum heif_filetype_result
    filetype_check;

  heif_item_id
    primary_image_id;

  Image
    *image;

  MagickBooleanType
    status;

  ssize_t
    count;

  struct heif_context
    *heif_context;

  struct heif_error
    error;

  struct heif_image_handle
    *image_handle;

  unsigned char
    magic[128];

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
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  if (ReadBlob(image,sizeof(magic),magic) != sizeof(magic))
    ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
  filetype_check=heif_check_filetype(magic,sizeof(magic));
  if (filetype_check == heif_filetype_no)
    ThrowReaderException(CoderError,"ImageTypeNotSupported");
  (void) CloseBlob(image);
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,11,0)
  if (heif_has_compatible_brand(magic,sizeof(magic), "avif") == 1)
    (void) CopyMagickString(image->magick,"AVIF",MagickPathExtent);
#endif
  /*
    Decode HEIF image.
  */
  heif_context=heif_context_alloc();
  if (heif_context == (struct heif_context *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,19,0)
  HEICSecurityLimits(image_info,heif_context);
#endif
  error=heif_context_read_from_file(heif_context,image->filename,
    (const struct heif_reading_options *) NULL);
  if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
    {
      heif_context_free(heif_context);
      return(DestroyImageList(image));
    }
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,20,0)
  /*
    Check for image sequence (animated AVIF) and decode via track API.
  */
  if (heif_context_has_sequence(heif_context) != 0)
    {
      status=ReadHEICSequenceFrames(image_info,image,heif_context,exception);
      heif_context_free(heif_context);
      if (status == MagickFalse)
        return(DestroyImageList(image));
      return(GetFirstImageInList(image));
    }
#endif
  error=heif_context_get_primary_image_ID(heif_context,&primary_image_id);
  if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
    {
      heif_context_free(heif_context);
      return(DestroyImageList(image));
    }
  error=heif_context_get_image_handle(heif_context,primary_image_id,
    &image_handle);
  if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
    {
      heif_context_free(heif_context);
      return(DestroyImageList(image));
    }
  status=ReadHEICImageHandle(image_info,image,heif_context,image_handle,
    exception);
  heif_image_handle_release(image_handle);
  count=(ssize_t) heif_context_get_number_of_top_level_images(heif_context);
  if ((status != MagickFalse) && (count > 1))
    {
      heif_item_id
        *ids;

      ssize_t
        i;