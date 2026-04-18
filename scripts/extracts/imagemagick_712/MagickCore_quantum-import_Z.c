// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 26/33



      range=GetQuantumRange(quantum_info->depth);
      if (quantum_info->pack == MagickFalse)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            SetPixelRed(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,range),q);
            SetPixelGreen(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,range),
              q);
            SetPixelBlue(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,range),q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
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
    case 12:
    {
      range=GetQuantumRange(quantum_info->depth);
      if (quantum_info->pack == MagickFalse)
        {
          unsigned short
            pixel;

          for (x=0; x < (ssize_t) (3*number_pixels-1); x+=2)
          {
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            switch (x % 3)
            {
              default:
              case 0:
              {
                SetPixelRed(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
                  range),q);
                break;
              }
              case 1:
              {
                SetPixelGreen(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
                  range),q);
                break;
              }
              case 2:
              {
                SetPixelBlue(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
                  range),q);
                q+=(ptrdiff_t) GetPixelChannels(image);
                break;
              }
            }
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            switch ((x+1) % 3)
            {
              default:
              case 0:
              {
                SetPixelRed(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
                  range),q);
                break;
              }
              case 1:
              {
                SetPixelGreen(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
                  range),q);
                break;
              }
              case 2:
              {
                SetPixelBlue(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
                  range),q);
                q+=(ptrdiff_t) GetPixelChannels(image);
                break;
              }
            }
            p+=(ptrdiff_t) quantum_info->pad;
          }
          for (bit=0; bit < (ssize_t) (3*number_pixels % 2); bit++)
          {
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            switch ((x+bit) % 3)
            {
              default:
              case 0:
              {
                SetPixelRed(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
                  range),q);
                break;
              }
              case 1:
              {
                SetPixelGreen(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
                  range),q);
                break;
              }
              case 2:
              {
                SetPixelBlue(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
                  range),q);
                q+=(ptrdiff_t) GetPixelChannels(image);
                break;
              }
            }
            p+=(ptrdiff_t) quantum_info->pad;
          }
          if (bit != 0)
            p++;
          break;
        }
      else
        {
          unsigned int
            pixel;