// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 3/17



  security_limits=heif_context_get_security_limits(heif_context);
  width_limit=(int) MagickMin(GetMagickResourceLimit(HeightResource),INT_MAX);
  height_limit=(int) MagickMin(GetMagickResourceLimit(WidthResource),INT_MAX);
  if (width_limit != INT_MAX || height_limit != INT_MAX)
    security_limits->max_image_size_pixels=(uint64_t) width_limit*height_limit;
  max_profile_size=(int) MagickMin(GetMaxProfileSize(),INT_MAX);
  if (max_profile_size != INT_MAX)
    security_limits->max_color_profile_size=max_profile_size;
  security_limits->max_memory_block_size=(uint64_t) GetMaxMemoryRequest();
  HEICSetUint64SecurityLimit(image_info,"heic:max-number-of-tiles",
    &security_limits->max_number_of_tiles);
  HEICSetUint32SecurityLimit(image_info,"heic:max-bayer-pattern-pixels",
    &security_limits->max_bayer_pattern_pixels);
  HEICSetUint32SecurityLimit(image_info,"heic:max-items",
    &security_limits->max_items);
  HEICSetUint32SecurityLimit(image_info,"heic:max-components",
    &security_limits->max_components);
  HEICSetUint32SecurityLimit(image_info,"heic:max-iloc-extents-per-item",
    &security_limits->max_iloc_extents_per_item);
  HEICSetUint32SecurityLimit(image_info,"heic:max-size-entity-group",
    &security_limits->max_size_entity_group);
  HEICSetUint32SecurityLimit(image_info,"heic:max-children-per-box",
    &security_limits->max_children_per_box);
}
#endif

static inline MagickBooleanType HEICSkipImage(const ImageInfo *image_info,
  Image *image)
{
  if (image_info->number_scenes == 0)
    return(MagickFalse);
  if (image->scene == 0)
    return(MagickFalse);
  if (image->scene < image_info->scene)
    return(MagickTrue);
  if (image->scene > image_info->scene+image_info->number_scenes-1)
    return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IsHEIFSuccess(Image *image,
  struct heif_error *error,ExceptionInfo *exception)
{
  if (error->code == 0)
    return(MagickTrue);
  (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
    error->message,"(%d.%d) `%s'",error->code,error->subcode,image->filename);
  return(MagickFalse);
}

static MagickBooleanType ReadHEICColorProfile(Image *image,
  struct heif_image_handle *image_handle,ExceptionInfo *exception)
{
  size_t
    length;

  struct heif_error
    error;

  unsigned char
    *color_profile;

  /*
    Read color profile.
  */
  length=heif_image_handle_get_raw_color_profile_size(image_handle);
  if (length == 0)
    return(MagickTrue);
  if ((MagickSizeType) length > GetBlobSize(image))
    ThrowBinaryException(CorruptImageError,"InsufficientImageDataInFile",
      image->filename);
  color_profile=(unsigned char *) AcquireQuantumMemory(1,length);
  if (color_profile == (unsigned char *) NULL)
    return(MagickFalse);
  error=heif_image_handle_get_raw_color_profile(image_handle,color_profile);
  if (IsHEIFSuccess(image,&error,exception) != MagickFalse)
    {
      StringInfo
        *profile;

      profile=BlobToProfileStringInfo("icc",color_profile,length,exception);
      (void) SetImageProfilePrivate(image,profile,exception);
    }
  color_profile=(unsigned char *) RelinquishMagickMemory(color_profile);
  return(MagickTrue);
}

static MagickBooleanType ReadHEICExifProfile(Image *image,
  struct heif_image_handle *image_handle,ExceptionInfo *exception)
{
  heif_item_id
    id;

  int
    count;

  size_t
    length;

  StringInfo
    *exif_profile;

  struct heif_error
    error;

  /*
    Read Exif profile.
  */
  count=heif_image_handle_get_list_of_metadata_block_IDs(image_handle,"Exif",
    &id,1);
  if (count != 1)
    return(MagickTrue);
  length=heif_image_handle_get_metadata_size(image_handle,id);
  if (length <= 8)
    return(MagickTrue);
  if ((MagickSizeType) length > GetBlobSize(image))
    ThrowBinaryException(CorruptImageError,"InsufficientImageDataInFile",
      image->filename);
  exif_profile=AcquireProfileStringInfo("exif",length,exception);
  if (exif_profile == (StringInfo*) NULL)
    return(MagickTrue);
  error=heif_image_handle_get_metadata(image_handle,id,
    GetStringInfoDatum(exif_profile));
  if ((IsHEIFSuccess(image,&error,exception) != MagickFalse) && (length > 4))
    {
      StringInfo
        *snippet = SplitStringInfo(exif_profile,4);

      unsigned char
        *datum;

      unsigned int
        offset = 0;