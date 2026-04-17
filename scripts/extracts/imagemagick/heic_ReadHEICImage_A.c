/* ===== EXTRACT: heic.c ===== */
/* Function: ReadHEICImage (part A) */
/* Lines: 615–747 */

static Image *ReadHEICImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  enum heif_filetype_result
    filetype_check;

  heif_item_id
    primary_image_id;

  Image
    *image;

  int
    max_size;

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
  max_size=(int) MagickMin(MagickMin(GetMagickResourceLimit(WidthResource),
    GetMagickResourceLimit(HeightResource)),INT_MAX);
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,19,0)
  if (max_size != INT_MAX)
    heif_context_set_maximum_image_size_limit(heif_context,max_size);
#endif
  error=heif_context_read_from_file(heif_context,image->filename,
    (const struct heif_reading_options *) NULL);
  if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
    {
      heif_context_free(heif_context);
      return(DestroyImageList(image));
    }
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
  status=ReadHEICImageHandle(image_info,image,image_handle,exception);
  heif_image_handle_release(image_handle);
  count=(ssize_t) heif_context_get_number_of_top_level_images(heif_context);
  if ((status != MagickFalse) && (count > 1))
    {
      heif_item_id
        *ids;

      ssize_t
        i;

      ids=(heif_item_id *) AcquireQuantumMemory((size_t) count,sizeof(*ids));
      if (ids == (heif_item_id *) NULL)
        {
          heif_context_free(heif_context);
          return(DestroyImageList(image));
        }
      (void) heif_context_get_list_of_top_level_image_IDs(heif_context,ids,
        (int) count);
      for (i=0; i < count; i++)
      {
        if (ids[i] == primary_image_id)
          continue;
        /*
          Allocate next image structure.
        */
        AcquireNextImage(image_info,image,exception);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            status=MagickFalse;
            break;
          }
        image=SyncNextImageInList(image);
        error=heif_context_get_image_handle(heif_context,ids[i],&image_handle);
        if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
          {
            status=MagickFalse;
            break;
          }
        status=ReadHEICImageHandle(image_info,image,image_handle,exception);
        heif_image_handle_release(image_handle);
        if (status == MagickFalse)
          break;
        if (image_info->number_scenes != 0)
