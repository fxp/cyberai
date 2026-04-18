// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 27/33



          if (quantum_info->quantum == 32U)
            {
              for (x=0; x < (ssize_t) number_pixels; x++)
              {
                p=PushQuantumLongPixel(quantum_info,p,&pixel);
                SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
                p=PushQuantumLongPixel(quantum_info,p,&pixel);
                SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
                p=PushQuantumLongPixel(quantum_info,p,&pixel);
                SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
                q+=(ptrdiff_t) GetPixelChannels(image);
              }
              break;
            }
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushQuantumPixel(quantum_info,p,&pixel);
            SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
            p=PushQuantumPixel(quantum_info,p,&pixel);
            SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
            p=PushQuantumPixel(quantum_info,p,&pixel);
            SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
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
            SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),q);
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),q);
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelRed(image,ScaleShortToQuantum(pixel),q);
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
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
            SetPixelRed(image,ClampToQuantum(pixel),q);
            p=PushQuantumFloatPixel(quantum_info,p,&pixel);
            SetPixelGreen(image,ClampToQuantum(pixel),q);
            p=PushQuantumFloatPixel(quantum_info,p,&pixel);
            SetPixelBlue(image,ClampToQuantum(pixel),q);
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
            SetPixelRed(image,ScaleLongToQuantum(pixel),q);
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            SetPixelGreen(image,ScaleLongToQuantum(pixel),q);
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            SetPixelBlue(image,ScaleLongToQuantum(pixel),q);
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
            SetPixelRed(image,ClampToQuantum(pixel),q);
            p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
            SetPixelGreen(image,ClampToQuantum(pixel),q);
            p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
            SetPixelBlue(image,ClampToQuantum(pixel),q);
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