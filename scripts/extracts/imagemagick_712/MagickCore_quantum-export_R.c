// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-export.c
// Segment 18/30



            float_pixel=(float) GetPixelLuma(image,p);
            q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
            float_pixel=(float) (GetPixelAlpha(image,p));
            q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=ScaleQuantumToLong(ClampToQuantum(GetPixelLuma(image,p)));
        q=PopLongPixel(quantum_info->endian,pixel,q);
        pixel=ScaleQuantumToLong(GetPixelAlpha(image,p));
        q=PopLongPixel(quantum_info->endian,pixel,q);
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
            double
              pixel;

            pixel=GetPixelLuma(image,p);
            q=PopQuantumDoublePixel(quantum_info,pixel,q);
            pixel=(double) (GetPixelAlpha(image,p));
            q=PopQuantumDoublePixel(quantum_info,pixel,q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(ClampToQuantum(
          GetPixelLuma(image,p)),range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelAlpha(image,p),
          range),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
  }
}

static void ExportIndexQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const Quantum *magick_restrict p,
  unsigned char *magick_restrict q,ExceptionInfo *exception)
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

      for (x=((ssize_t) number_pixels-7); x > 0; x-=8)
      {
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q=((pixel & 0x01) << 7);
        p+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0x01) << 6);
        p+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0x01) << 5);
        p+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0x01) << 4);
        p+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0x01) << 3);
        p+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0x01) << 2);
        p+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0x01) << 1);
        p+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0x01) << 0);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q++;
      }
      if ((number_pixels % 8) != 0)
        {
          *q='\0';
          for (bit=7; bit >= (ssize_t) (8-(number_pixels % 8)); bit--)
          {
            pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
            *q|=((pixel & 0x01) << (unsigned char) bit);
            p+=(ptrdiff_t) GetPixelChannels(image);
          }
          q++;
        }
      break;
    }
    case 4:
    {
      unsigned char
        pixel;