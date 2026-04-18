// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 12/17



  uint8_t
    *pixels,
    *q;

  /*
    Transform HEIF RGBA image.
  */
  status=MagickTrue;
  channel=heif_channel_interleaved;
  if (GetPixelChannels(image) == 1)
    channel=heif_channel_Y;
  error=heif_image_add_plane(heif_image,channel,(int) image->columns,
    (int) image->rows,8);
  status=IsHEIFSuccess(image,&error,exception);
  if (status == MagickFalse)
    return(status);
  pixels=heif_image_get_plane(heif_image,channel,&stride);
  if (pixels == (uint8_t *) NULL)
    return(MagickFalse);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        break;
      }
    q=pixels+(y*stride);
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      *(q++)=ScaleQuantumToChar(GetPixelRed(image,p));
      if (GetPixelChannels(image) > 1)
        {
          *(q++)=ScaleQuantumToChar(GetPixelGreen(image,p));
          *(q++)=ScaleQuantumToChar(GetPixelBlue(image,p));
          if ((image->alpha_trait & BlendPixelTrait) != 0)
            *(q++)=ScaleQuantumToChar(GetPixelAlpha(image,p));
        }
      p+=(ptrdiff_t) GetPixelChannels(image);
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

static MagickBooleanType WriteHEICImageRRGGBBAA(Image *image,
  struct heif_image *heif_image,ExceptionInfo *exception)
{
  const Quantum
    *p;

  enum heif_channel
    channel = heif_channel_interleaved;

  int
    depth,
    shift,
    stride;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  struct heif_error
    error;

  uint8_t
    *pixels,
    *q;

  /*
    Transform HEIF RGBA image with depth > 8.
  */
  depth=image->depth > 10 ? 12 : 10;
  if (GetPixelChannels(image) == 1)
    channel=heif_channel_Y;
  error=heif_image_add_plane(heif_image,channel,(int) image->columns,
    (int) image->rows,depth);
  if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
    return(MagickFalse);
  status=IsHEIFSuccess(image,&error,exception);
  if (status == MagickFalse)
    return(status);
  pixels=heif_image_get_plane(heif_image,channel,&stride);
  if (pixels == (uint8_t *) NULL)
    return(MagickFalse);
  shift=(int) (16-depth);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        break;
      }
    q=pixels+(y*stride);
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      int pixel=ScaleQuantumToShort(GetPixelRed(image,p)) >> shift;
      *(q++)=(uint8_t) (pixel & 0xff);
      *(q++)=(uint8_t) (pixel >> 8);
      if (GetPixelChannels(image) > 1)
        {
          pixel=ScaleQuantumToShort(GetPixelGreen(image,p)) >> shift;
          *(q++)=(uint8_t) (pixel & 0xff);
          *(q++)=(uint8_t) (pixel >> 8);
          pixel=ScaleQuantumToShort(GetPixelBlue(image,p)) >> shift;
          *(q++)=(uint8_t) (pixel & 0xff);
          *(q++)=(uint8_t) (pixel >> 8);
          if ((image->alpha_trait & BlendPixelTrait) != 0)
            {
              pixel=ScaleQuantumToShort(GetPixelAlpha(image,p)) >> shift;
              *(q++)=(uint8_t) (pixel & 0xff);
              *(q++)=(uint8_t) (pixel >> 8);
            }
        }
      p+=(ptrdiff_t) GetPixelChannels(image);
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

#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,20,0)
static MagickBooleanType WriteHEICSequenceImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  const char
    *option;

  enum heif_chroma
    chroma;

  enum heif_colorspace
    colorspace;

  heif_track
    *track = (heif_track *) NULL;

  MagickBooleanType
    lossless,
    status;

  MagickOffsetType
    scene;

  struct heif_context
    *heif_context;

  struct heif_encoder
    *heif_encoder = (struct heif_encoder *) NULL;

  struct heif_error
    error;

  struct heif_image
    *heif_image = (struct heif_image *) NULL;

  struct heif_sequence_encoding_options
    *seq_options = (struct heif_sequence_encoding_options *) NULL;

  struct heif_track_options
    *track_options;

  uint32_t
    timescale;