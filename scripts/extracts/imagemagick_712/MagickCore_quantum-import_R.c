// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 18/33



      if (quantum_info->min_is_white != MagickFalse)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,QuantumRange-ScaleShortToQuantum(pixel),q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelGray(image,ScaleShortToQuantum(pixel),q);
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
            SetPixelGray(image,ClampToQuantum(pixel),q);
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
            SetPixelGray(image,ScaleLongToQuantum(pixel),q);
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
            SetPixelGray(image,ClampToQuantum(pixel),q);
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
            SetPixelGray(image,ClampToQuantum(pixel),q);
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
        SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
  }
}

static void ImportGrayAlphaQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const unsigned char *magick_restrict p,
  Quantum *magick_restrict q)
{
  QuantumAny
    range;

  ssize_t
    x;

  ssize_t
    bit;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 1:
    {
      unsigned char
        pixel;

      bit=0;
      for (x=((ssize_t) number_pixels-3); x > 0; x-=4)
      {
        for (bit=0; bit < 8; bit+=2)
        {
          pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x00 : 0x01);
          SetPixelGray(image,(Quantum) (pixel == 0 ? 0 : QuantumRange),q);
          SetPixelAlpha(image,((*p) & (1UL << (unsigned char) (6-bit))) == 0 ?
            TransparentAlpha : OpaqueAlpha,q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        p++;
      }
      if ((number_pixels % 4) != 0)
        for (bit=3; bit >= (ssize_t) (4-(number_pixels % 4)); bit-=2)
        {
          pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x00 : 0x01);
          SetPixelGray(image,(Quantum) (pixel != 0 ? 0 : QuantumRange),q);
          SetPixelAlpha(image,((*p) & (1UL << (unsigned char) (6-bit))) == 0 ?
            TransparentAlpha : OpaqueAlpha,q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
      if (bit != 0)
        p++;
      break;
    }
    case 4:
    {
      unsigned char
        pixel;