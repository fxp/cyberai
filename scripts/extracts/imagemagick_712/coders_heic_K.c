// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 11/17

:
%
%      MagickBooleanType WriteHEICImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: the image info.
%
%    o image:  The image.
%
%    o exception:  return any errors or warnings in this structure.
%
*/

#if defined(MAGICKCORE_HEIC_DELEGATE)
static void WriteProfile(struct heif_context *context,Image *image,
  ExceptionInfo *exception)
{
  const char
    *name;

  const StringInfo
    *profile;

  size_t
    length;

  ssize_t
    i;

  struct heif_error
    error;

  struct heif_image_handle
    *image_handle;

  /*
    Get image handle.
  */
  image_handle=(struct heif_image_handle *) NULL;
  error=heif_context_get_primary_image_handle(context,&image_handle);
  if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
    return;
  /*
    Save image profile as a APP marker.
  */
  ResetImageProfileIterator(image);
  for (name=GetNextImageProfile(image); name != (const char *) NULL; )
  {
    profile=GetImageProfile(image,name);
    length=GetStringInfoLength(profile);
    if (LocaleCompare(name,"EXIF") == 0)
      (void) heif_context_add_exif_metadata(context,image_handle,
        (void*) GetStringInfoDatum(profile),(int) length);
    if (LocaleCompare(name,"XMP") == 0)
      for (i=0; i < (ssize_t) GetStringInfoLength(profile); i+=65533L)
      {
        length=(size_t) MagickMin((ssize_t) GetStringInfoLength(profile)-i,
          65533L);
        error=heif_context_add_XMP_metadata(context,image_handle,
          (void*) (GetStringInfoDatum(profile)+i),(int) length);
        if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
          break;
      }
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "%s profile: %.20g bytes",name,(double) GetStringInfoLength(profile));
    name=GetNextImageProfile(image);
  }
  heif_image_handle_release(image_handle);
}

static struct heif_error heif_write_func(struct heif_context *context,
  const void* data,size_t size,void* userdata)
{
  Image
    *image;

  struct heif_error
    error_ok;

  (void) context;
  image=(Image*) userdata;
  (void) WriteBlob(image,size,(const unsigned char *) data);
  error_ok.code=heif_error_Ok;
  error_ok.subcode=heif_suberror_Unspecified;
  error_ok.message="ok";
  return(error_ok);
}

static MagickBooleanType WriteHEICImageYCbCr(Image *image,
  struct heif_image *heif_image,ExceptionInfo *exception)
{
  int
    p_cb,
    p_cr,
    p_y;

  MagickBooleanType
    status;

  ssize_t
    y;

  struct heif_error
    error;

  uint8_t
    *q_cb,
    *q_cr,
    *q_y;

  /*
    Transform HEIF YCbCr image.
  */
  status=MagickTrue;
  error=heif_image_add_plane(heif_image,heif_channel_Y,(int) image->columns,
    (int) image->rows,8);
  status=IsHEIFSuccess(image,&error,exception);
  if (status == MagickFalse)
    return(status);
  error=heif_image_add_plane(heif_image,heif_channel_Cb,
    ((int) image->columns+1)/2,((int) image->rows+1)/2,8);
  status=IsHEIFSuccess(image,&error,exception);
  if (status == MagickFalse)
    return(status);
  error=heif_image_add_plane(heif_image,heif_channel_Cr,
    ((int) image->columns+1)/2,((int) image->rows+1)/2,8);
  status=IsHEIFSuccess(image,&error,exception);
  if (status == MagickFalse)
    return(status);
  q_y=heif_image_get_plane(heif_image,heif_channel_Y,&p_y);
  q_cb=heif_image_get_plane(heif_image,heif_channel_Cb,&p_cb);
  q_cr=heif_image_get_plane(heif_image,heif_channel_Cr,&p_cr);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *p;

    ssize_t
      x;

    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        break;
      }
    if ((y & 0x01) != 0)
      for (x=0; x < (ssize_t) image->columns; x++)
      {
        q_y[y*p_y+x]=ScaleQuantumToChar(GetPixelRed(image,p));
        p+=(ptrdiff_t) GetPixelChannels(image);
      }
    else
      for (x=0; x < (ssize_t) image->columns; x+=2)
      {
        q_y[y*p_y+x]=ScaleQuantumToChar(GetPixelRed(image,p));
        q_cb[y/2*p_cb+x/2]=ScaleQuantumToChar(GetPixelGreen(image,p));
        q_cr[y/2*p_cr+x/2]=ScaleQuantumToChar(GetPixelBlue(image,p));
        p+=(ptrdiff_t) GetPixelChannels(image);
        if ((x+1) < (ssize_t) image->columns)
          {
            q_y[y*p_y+x+1]=ScaleQuantumToChar(GetPixelRed(image,p));
            p+=(ptrdiff_t) GetPixelChannels(image);
          }
      }
    if (image->previous == (Image *) NULL)
      {
        status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
          image->rows);
        if (status == MagickFalse)
          break;
      }
  }
  return(status);
}

static MagickBooleanType WriteHEICImageRGBA(Image *image,
  struct heif_image *heif_image,ExceptionInfo *exception)
{
  const Quantum
    *p;

  enum heif_channel
    channel;

  int
    stride;

  MagickBooleanType
    status;

  ssize_t
    y;

  struct heif_error
    error;