// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-export.c
// Segment 19/30



      for (x=0; x < ((ssize_t) number_pixels-1) ; x+=2)
      {
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q=((pixel & 0xf) << 4);
        p+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0xf) << 0);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q++;
      }
      if ((number_pixels % 2) != 0)
        {
          pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
          *q=((pixel & 0xf) << 4);
          p+=(ptrdiff_t) GetPixelChannels(image);
          q++;
        }
      break;
    }
    case 8:
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopCharPixel((unsigned char) ((ssize_t) GetPixelIndex(image,p)),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
    case 16:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            q=PopShortPixel(quantum_info->endian,SinglePrecisionToHalf(
              QuantumScale*(double) GetPixelIndex(image,p)),q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopShortPixel(quantum_info->endian,(unsigned short)
          GetPixelIndex(image,p),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
    case 32:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            q=PopQuantumFloatPixel(quantum_info,(float) GetPixelIndex(image,p),q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopLongPixel(quantum_info->endian,(unsigned int)
          GetPixelIndex(image,p),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            q=PopQuantumDoublePixel(quantum_info,(double) GetPixelIndex(image,p),q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopQuantumPixel(quantum_info,(QuantumAny) GetPixelIndex(image,p),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
  }
}

static void ExportIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const MagickSizeType number_pixels,
  const Quantum *magick_restrict p,unsigned char *magick_restrict q,
  ExceptionInfo *exception)
{
  ssize_t
    x;

  ssize_t
    bit;

  if (image->storage_class != PseudoClass)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColormappedImageRequired","`%s'",image->filename);
      return;
    }
  switch (quantum_info->depth)
  {
    case 1:
    {
      unsigned char
        pixel;