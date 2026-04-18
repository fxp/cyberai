// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 3/33



  *quantum=0UL;
  for (i=(ssize_t) quantum_info->depth; i > 0; )
  {
    if (quantum_info->state.bits == 0)
      {
        pixels=PushLongPixel(quantum_info->endian,pixels,
          &quantum_info->state.pixel);
        quantum_info->state.bits=32U;
      }
    quantum_bits=(size_t) i;
    if (quantum_bits > quantum_info->state.bits)
      quantum_bits=quantum_info->state.bits;
    *quantum|=(((quantum_info->state.pixel >> (32U-quantum_info->state.bits)) &
      quantum_info->state.mask[quantum_bits]) << ((ssize_t)
      quantum_info->depth-i));
    i-=(ssize_t) quantum_bits;
    quantum_info->state.bits-=quantum_bits;
  }
  return(pixels);
}

static void ImportPixelChannel(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const unsigned char *magick_restrict p,
  Quantum *magick_restrict q,PixelChannel channel)
{
  QuantumAny
    range;

  ssize_t
    x;

  q+=(ptrdiff_t) image->channel_map[channel].offset;
  switch (quantum_info->depth)
  {
    case 8:
    {
      unsigned char
        pixel;

      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushCharPixel(p,&pixel);
        *q=ScaleCharToQuantum(pixel);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 16:
    {
      unsigned short
        pixel;

      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            *q=ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel));
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        *q=ScaleShortToQuantum(pixel);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 32:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          float
            pixel;

          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushQuantumFloatPixel(quantum_info,p,&pixel);
            *q=ClampToQuantum(pixel);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      else
        {
          unsigned int
            pixel;

          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            *q=ScaleLongToQuantum(pixel);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
    }
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          float
            pixel;

          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
            *q=ClampToQuantum(pixel);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          double
            pixel;

          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushDoublePixel(quantum_info,p,&pixel);
            *q=ClampToQuantum(pixel);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      unsigned int
        pixel;

      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushQuantumPixel(quantum_info,p,&pixel);
        *q=ScaleAnyToQuantum(pixel,range);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
  }
}

static void ImportAlphaQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const unsigned char *magick_restrict p,
  Quantum *magick_restrict q,ExceptionInfo *exception)
{
  if (image->alpha_trait == UndefinedPixelTrait)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ImageDoesNotHaveAnAlphaChannel","`%s'",image->filename);
      return;
    }
  ImportPixelChannel(image,quantum_info,number_pixels,p,q,AlphaPixelChannel);
}

static void ImportBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const unsigned char *magick_restrict p,
  Quantum *magick_restrict q)
{
  QuantumAny
    range;

  ssize_t
    x;

  ssize_t
    bit;