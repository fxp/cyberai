// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 17/33



      pixel=0;
      range=GetQuantumRange(quantum_info->depth);
      if (quantum_info->pack == MagickFalse)
        {
          if (image->endian == LSBEndian)
            {
              for (x=0; x < ((ssize_t) number_pixels-2); x+=3)
              {
                p=PushLongPixel(quantum_info->endian,p,&pixel);
                SetPixelGray(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,
                  range),q);
                q+=(ptrdiff_t) GetPixelChannels(image);
                SetPixelGray(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,
                  range),q);
                q+=(ptrdiff_t) GetPixelChannels(image);
                SetPixelGray(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,
                  range),q);
                p+=(ptrdiff_t) quantum_info->pad;
                q+=(ptrdiff_t) GetPixelChannels(image);
              }
              if (x++ < ((ssize_t) number_pixels-1))
                {
                  p=PushLongPixel(quantum_info->endian,p,&pixel);
                  SetPixelGray(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,
                    range),q);
                  q+=(ptrdiff_t) GetPixelChannels(image);
                }
              if (x++ < (ssize_t) number_pixels)
                {
                  SetPixelGray(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,
                    range),q);
                  q+=(ptrdiff_t) GetPixelChannels(image);
                }
              break;
            }
          for (x=0; x < ((ssize_t) number_pixels-2); x+=3)
          {
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,range),
              q);
            q+=(ptrdiff_t) GetPixelChannels(image);
            SetPixelGray(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,range),
              q);
            q+=(ptrdiff_t) GetPixelChannels(image);
            SetPixelGray(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,range),
              q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          if (x++ < ((ssize_t) number_pixels-1))
            {
              p=PushLongPixel(quantum_info->endian,p,&pixel);
              SetPixelGray(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,
                range),q);
              q+=(ptrdiff_t) GetPixelChannels(image);
            }
          if (x++ < (ssize_t) number_pixels)
            {
              SetPixelGray(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,
                range),q);
              q+=(ptrdiff_t) GetPixelChannels(image);
            }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 12:
    {
      range=GetQuantumRange(quantum_info->depth);
      if (quantum_info->pack == MagickFalse)
        {
          unsigned short
            pixel;

          for (x=0; x < ((ssize_t) number_pixels-1); x+=2)
          {
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
              range),q);
            q+=(ptrdiff_t) GetPixelChannels(image);
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
              range),q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          for (bit=0; bit < (ssize_t) (number_pixels % 2); bit++)
          {
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
              range),q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          if (bit != 0)
            p++;
          break;
        }
      else
        {
          unsigned int
            pixel;

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
    case 16:
    {
      unsigned short
        pixel;