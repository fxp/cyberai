// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 4/17



      /*
        Extract Exif profile.
      */
      datum=GetStringInfoDatum(snippet);
      offset|=(unsigned int) (*(datum++)) << 24;
      offset|=(unsigned int) (*(datum++)) << 16;
      offset|=(unsigned int) (*(datum++)) << 8;
      offset|=(unsigned int) (*(datum++)) << 0;
      snippet=DestroyStringInfo(snippet);
      /*
        Strip any EOI marker if payload starts with a JPEG marker.
      */
      length=GetStringInfoLength(exif_profile);
      datum=GetStringInfoDatum(exif_profile);
      if ((length > 2) &&
          ((memcmp(datum,"\xff\xd8",2) == 0) ||
           (memcmp(datum,"\xff\xe1",2) == 0)) &&
           (memcmp(datum+length-2,"\xff\xd9",2) == 0))
        SetStringInfoLength(exif_profile,length-2);
      /*
        Skip to actual Exif payload.
      */
      if (offset < GetStringInfoLength(exif_profile))
        {
          (void) DestroyStringInfo(SplitStringInfo(exif_profile,offset));
          (void) SetImageProfilePrivate(image,exif_profile,exception);
          exif_profile=(StringInfo *) NULL;
        }
    }
  if (exif_profile != (StringInfo *) NULL)
    exif_profile=DestroyStringInfo(exif_profile);
  return(MagickTrue);
}

static MagickBooleanType ReadHEICXMPProfile(Image *image,
  struct heif_image_handle *image_handle,ExceptionInfo *exception)
{
  heif_item_id
    id;

  int
    count;

  size_t
    length;

  struct heif_error
    error;

  unsigned char
    *xmp_profile;

  /*
    Read XMP profile.
  */
  count=heif_image_handle_get_list_of_metadata_block_IDs(image_handle,"mime",
    &id,1);
  if (count != 1)
    return(MagickTrue);
  length=heif_image_handle_get_metadata_size(image_handle,id);
  if (length <= 8)
    return(MagickTrue);
  if ((MagickSizeType) length > GetBlobSize(image))
    ThrowBinaryException(CorruptImageError,"InsufficientImageDataInFile",
      image->filename);
  xmp_profile=(unsigned char *) AcquireQuantumMemory(1,length);
  if (xmp_profile == (unsigned char *) NULL)
    return(MagickFalse);
  error=heif_image_handle_get_metadata(image_handle,id,xmp_profile);
  if (IsHEIFSuccess(image,&error,exception) != MagickFalse)
    {
      StringInfo
        *profile;

      profile=BlobToProfileStringInfo("xmp",xmp_profile,length,exception);
      (void) SetImageProfilePrivate(image,profile,exception);
    }
  xmp_profile=(unsigned char *) RelinquishMagickMemory(xmp_profile);
  return(MagickTrue);
}

static MagickBooleanType ReadHEICImageHandle(const ImageInfo *image_info,
  Image *image,struct heif_context *heif_context,
  struct heif_image_handle *image_handle,ExceptionInfo *exception)
{
  const uint8_t
    *p,
    *pixels;

  enum heif_channel
    channel;

  enum heif_chroma
    chroma;

  int
    bits_per_pixel,
    shift,
    stride = 0;

  MagickBooleanType
    preserve_orientation,
    status;

  ssize_t
    y;

  struct heif_decoding_options
    *decode_options;

  struct heif_error
    error;

  struct heif_image
    *heif_image;

  /*
    Read HEIC image from container.
  */
  image->columns=(size_t) heif_image_handle_get_width(image_handle);
  image->rows=(size_t) heif_image_handle_get_height(image_handle);
  if (heif_image_handle_has_alpha_channel(image_handle) != 0)
    image->alpha_trait=BlendPixelTrait;
  image->depth=8;
  bits_per_pixel=heif_image_handle_get_luma_bits_per_pixel(image_handle);
  if (bits_per_pixel != -1)
    image->depth=(size_t) bits_per_pixel;
  preserve_orientation=IsStringTrue(GetImageOption(image_info,
    "heic:preserve-orientation"));
  if (preserve_orientation == MagickFalse)
    (void) SetImageProperty(image,"exif:Orientation","1",exception);
  else
    {
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,17,0)
      enum heif_item_property_type
        type = heif_item_property_type_invalid;

      heif_item_id
        item_id;

      heif_property_id
        transforms[1];

      int
        count;

      item_id=heif_image_handle_get_item_id(image_handle);
      count=heif_item_get_transformation_properties(heif_context,item_id,
        transforms,1);
      if (count == 1)
        type=heif_item_get_property_type(heif_context,item_id,transforms[0]);
      if (count == 1 && ((type == heif_item_property_type_transform_mirror) ||
          (type == heif_item_property_type_transform_rotation)))
        {
          enum heif_transform_mirror_direction
            mirror;

          int
            rotation_ccw;