// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-export.c
// Segment 21/30



            q=PopQuantumDoublePixel(quantum_info,(double) GetPixelIndex(image,p),q);
            pixel=(double) GetPixelAlpha(image,p);
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
      QuantumAny
        range;

      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopQuantumPixel(quantum_info,(QuantumAny) GetPixelIndex(image,p),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelAlpha(image,p),
          range),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
  }
}

static void ExportMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const MagickSizeType number_pixels,
  const Quantum *magick_restrict p,unsigned char *magick_restrict q,
  ExceptionInfo *exception)
{
  ssize_t
    i,
    x;

  if (image->number_meta_channels == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "MultispectralImageRequired","`%s'",image->filename);
      return;
    }
  if (quantum_info->meta_channel != 0)
    {
      ExportPixelChannel(image,quantum_info,number_pixels,p,q,
        (PixelChannel) (MetaPixelChannels+quantum_info->meta_channel-1));
      return;
    }
  switch (quantum_info->depth)
  {
    case 8:
    {
      unsigned char
        pixel;

      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        for (i=0; i < (ssize_t) GetImageChannels(image); i++)
        {
          pixel=ScaleQuantumToChar(p[i]);
          q=PopCharPixel(pixel,q);
        }
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
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
            for (i=0; i < (ssize_t) GetImageChannels(image); i++)
            {
              pixel=SinglePrecisionToHalf(QuantumScale*(double) p[i]);
              q=PopShortPixel(quantum_info->endian,pixel,q);
            }
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        for (i=0; i < (ssize_t) GetImageChannels(image); i++)
        {
          pixel=ScaleQuantumToShort(p[i]);
          q=PopShortPixel(quantum_info->endian,pixel,q);
        }
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
    case 32:
    {
      unsigned int
        pixel;

      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            for (i=0; i < (ssize_t) GetImageChannels(image); i++)
              q=PopQuantumFloatPixel(quantum_info,(float) p[i],q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        for (i=0; i < (ssize_t) GetImageChannels(image); i++)
        {
          pixel=ScaleQuantumToLong(p[i]);
          q=PopLongPixel(quantum_info->endian,pixel,q);
        }
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
            for (i=0; i < (ssize_t) GetImageChannels(image); i++)
              q=PopQuantumDoublePixel(quantum_info,(double) p[i],q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      QuantumAny
        range;

      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        for (i=0; i < (ssize_t) GetImageChannels(image); i++)
          q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(p[i],range),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
  }
}

static void ExportOpacityQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const Quantum *magick_restrict p,
  unsigned char *magick_restrict q,ExceptionInfo *exception)
{
  QuantumAny
    range;

  ssize_t
    x;