// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 6/17



      /*
        Transform 8-bit image.
      */
      q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
      if (q == (Quantum *) NULL)
        break;
      p=pixels+(y*stride);
      for (x=0; x < (ssize_t) image->columns; x++)
      {
        SetPixelRed(image,ScaleCharToQuantum((unsigned char) *(p++)),q);
        SetPixelGreen(image,ScaleCharToQuantum((unsigned char) *(p++)),q);
        SetPixelBlue(image,ScaleCharToQuantum((unsigned char) *(p++)),q);
        if (image->alpha_trait != UndefinedPixelTrait)
          SetPixelAlpha(image,ScaleCharToQuantum((unsigned char) *(p++)),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      if (SyncAuthenticPixels(image,exception) == MagickFalse)
        break;
    }
  else
    for (y=0; y < (ssize_t) image->rows; y++)
    {
      Quantum
        *q;

      ssize_t
        x;

      /*
        Transform 10-bit or 12-bit image.
      */
      q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
      if (q == (Quantum *) NULL)
        break;
      p=pixels+(y*stride);
      for (x=0; x < (ssize_t) image->columns; x++)
      {
        unsigned short pixel = (((unsigned short) *(p+1) << 8) |
          (*(p+0))) << shift; p+=(ptrdiff_t) 2;
        SetPixelRed(image,ScaleShortToQuantum(pixel),q);
        pixel=(((unsigned short) *(p+1) << 8) | (*(p+0))) << shift; p+=(ptrdiff_t) 2;
        SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
        pixel=(((unsigned short) *(p+1) << 8) | (*(p+0))) << shift; p+=(ptrdiff_t) 2;
        SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
        if (image->alpha_trait != UndefinedPixelTrait)
          {
            pixel=(((unsigned short) *(p+1) << 8) | (*(p+0))) << shift; p+=(ptrdiff_t) 2;
            SetPixelAlpha(image,ScaleShortToQuantum(pixel),q);
          }
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      if (SyncAuthenticPixels(image,exception) == MagickFalse)
        break;
    }
  heif_image_release(heif_image);
  return(MagickTrue);
}

#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,20,0)
static MagickBooleanType ReadHEICSequenceFrames(const ImageInfo *image_info,
  Image *image,struct heif_context *heif_context,ExceptionInfo *exception)
{
  const uint8_t
    *p,
    *pixels;

  enum heif_channel
    channel;

  enum heif_chroma
    chroma;

  heif_track
    *track;

  int
    bits_per_pixel,
    has_alpha = 0,
    shift,
    stride = 0;

  MagickBooleanType
    status;

  size_t
    scene;

  struct heif_decoding_options
    *decode_options;

  struct heif_error
    error;

  struct heif_image
    *heif_image;

  uint16_t
    track_width,
    track_height;

  uint32_t
    timescale;

  /*
    Get the first visual track from the sequence.
  */
  track=heif_context_get_track(heif_context,0);
  if (track == (heif_track *) NULL)
    return(MagickFalse);
  error=heif_track_get_image_resolution(track,&track_width,&track_height);
  if (error.code != 0)
    {
      heif_track_release(track);
      return(MagickFalse);
    }
  timescale=heif_track_get_timescale(track);
  if (timescale == 0)
    timescale=1;
  decode_options=heif_decoding_options_alloc();
  /*
    Detect alpha from the track and set up chroma format.
  */
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,21,0)
  has_alpha=heif_track_has_alpha_channel(track);
  image->alpha_trait=UndefinedPixelTrait;
  if (has_alpha != 0)
    image->alpha_trait=BlendPixelTrait;
#endif
  image->depth=8;
  if (image->alpha_trait != UndefinedPixelTrait)
    {
      chroma=heif_chroma_interleaved_RGBA;
      if (image->depth > 8)
        chroma=heif_chroma_interleaved_RRGGBBAA_LE;
    }
  else
    {
      chroma=heif_chroma_interleaved_RGB;
      if (image->depth > 8)
        chroma=heif_chroma_interleaved_RRGGBB_LE;
    }
  scene=0;
  status=MagickTrue;
  for ( ; ; )
  {
    ssize_t
      y;

    uint32_t
      duration;