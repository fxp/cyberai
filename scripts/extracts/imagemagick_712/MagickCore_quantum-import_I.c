// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 9/33



          n=0;
          quantum=0;
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            for (i=0; i < 4; i++)
            {
              switch (n % 3)
              {
                case 0:
                {
                  p=PushLongPixel(quantum_info->endian,p,&pixel);
                  quantum=(size_t) (ScaleShortToQuantum((unsigned short)
                    (((pixel >> 22) & 0x3ff) << 6)));
                  break;
                }
                case 1:
                {
                  quantum=(size_t) (ScaleShortToQuantum((unsigned short)
                    (((pixel >> 12) & 0x3ff) << 6)));
                  break;
                }
                case 2:
                {
                  quantum=(size_t) (ScaleShortToQuantum((unsigned short)
                    (((pixel >> 2) & 0x3ff) << 6)));
                  break;
                }
              }
              switch (i)
              {
                case 0: SetPixelRed(image,(Quantum) quantum,q); break;
                case 1: SetPixelGreen(image,(Quantum) quantum,q); break;
                case 2: SetPixelBlue(image,(Quantum) quantum,q); break;
                case 3: SetPixelOpacity(image,(Quantum) quantum,q); break;
              }
              n++;
            }
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelRed(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelGreen(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
          q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelBlue(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
          q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelOpacity(image,ScaleShortToQuantum((unsigned short)
          (pixel << 6)),q);
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
            SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),q);
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),q);
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),q);
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelOpacity(image,ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelRed(image,ScaleShortToQuantum(pixel),q);
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelOpacity(image,ScaleShortToQuantum(pixel),q);
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
            p=PushQuantumFloatPixel(quantum_info,p,&pixel);
            SetPixelOpacity(image,ClampToQuantum(pixel),q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      else
        {
          unsigned int
            pixel;