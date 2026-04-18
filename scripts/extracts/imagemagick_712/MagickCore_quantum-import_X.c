// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 24/33



      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            for (i=0; i < (ssize_t) GetImageChannels(image); i++)
            {
              p=PushShortPixel(quantum_info->endian,p,&pixel);
              q[i]=ClampToQuantum((double) QuantumRange*(double)HalfToSinglePrecision(pixel));
            }
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        for (i=0; i < (ssize_t) GetImageChannels(image); i++)
        {
          p=PushShortPixel(quantum_info->endian,p,&pixel);
          q[i]=ScaleShortToQuantum(pixel);
        }
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
            for (i=0; i < (ssize_t) GetImageChannels(image); i++)
            {
              p=PushQuantumFloatPixel(quantum_info,p,&pixel);
              q[i]=ClampToQuantum(pixel);
            }
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
            for (i=0; i < (ssize_t) GetImageChannels(image); i++)
            {
              p=PushLongPixel(quantum_info->endian,p,&pixel);
              q[i]=ScaleLongToQuantum(pixel);
            }
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
            for (i=0; i < (ssize_t) GetImageChannels(image); i++)
            {
              p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
              q[i]=ClampToQuantum(pixel);
            }
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
            for (i=0; i < (ssize_t) GetImageChannels(image); i++)
            {
              p=PushDoublePixel(quantum_info,p,&pixel);
              q[i]=ClampToQuantum(pixel);
            }
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
        pixel = 0;

      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        for (i=0; i < (ssize_t) GetImageChannels(image); i++)
        {
          p=PushQuantumPixel(quantum_info,p,&pixel);
          q[i]=ScaleAnyToQuantum(pixel,range);
        }
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
  }
}

static void ImportOpacityQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const unsigned char *magick_restrict p,
  Quantum *magick_restrict q,ExceptionInfo* exception)
{
  QuantumAny
    range;

  ssize_t
    x;

  if (image->alpha_trait == UndefinedPixelTrait)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ImageDoesNotHaveAnAlphaChannel","`%s'",image->filename);
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
        p=PushCharPixel(p,&pixel);
        SetPixelOpacity(image,ScaleCharToQuantum(pixel),q);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 16:
    {
      unsigned short
        pixel;