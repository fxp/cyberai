// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-export.c
// Segment 20/30



      for (x=((ssize_t) number_pixels-3); x > 0; x-=4)
      {
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q=((pixel & 0x01) << 7);
        pixel=(unsigned char) (GetPixelAlpha(image,p) == (Quantum)
          TransparentAlpha ? 1 : 0);
        *q|=((pixel & 0x01) << 6);
        p+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0x01) << 5);
        pixel=(unsigned char) (GetPixelAlpha(image,p) == (Quantum)
          TransparentAlpha ? 1 : 0);
        *q|=((pixel & 0x01) << 4);
        p+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0x01) << 3);
        pixel=(unsigned char) (GetPixelAlpha(image,p) == (Quantum)
          TransparentAlpha ? 1 : 0);
        *q|=((pixel & 0x01) << 2);
        p+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0x01) << 1);
        pixel=(unsigned char) (GetPixelAlpha(image,p) == (Quantum)
          TransparentAlpha ? 1 : 0);
        *q|=((pixel & 0x01) << 0);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q++;
      }
      if ((number_pixels % 4) != 0)
        {
          *q='\0';
          for (bit=3; bit >= (ssize_t) (4-(number_pixels % 4)); bit-=2)
          {
            pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
            *q|=((pixel & 0x01) << (unsigned char) (bit+4));
            pixel=(unsigned char) (GetPixelAlpha(image,p) == (Quantum)
              TransparentAlpha ? 1 : 0);
            *q|=((pixel & 0x01) << (unsigned char) (bit+4-1));
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

      for (x=0; x < (ssize_t) number_pixels ; x++)
      {
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q=((pixel & 0xf) << 4);
        pixel=(unsigned char) (16.0*QuantumScale*(double)
          GetPixelAlpha(image,p)+0.5);
        *q|=((pixel & 0xf) << 0);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q++;
      }
      break;
    }
    case 8:
    {
      unsigned char
        pixel;

      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopCharPixel((unsigned char) ((ssize_t) GetPixelIndex(image,p)),q);
        pixel=ScaleQuantumToChar(GetPixelAlpha(image,p));
        q=PopCharPixel(pixel,q);
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
            q=PopShortPixel(quantum_info->endian,(unsigned short)
              ((ssize_t) GetPixelIndex(image,p)),q);
            pixel=SinglePrecisionToHalf(QuantumScale*(double)
              GetPixelAlpha(image,p));
            q=PopShortPixel(quantum_info->endian,pixel,q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopShortPixel(quantum_info->endian,(unsigned short)
          ((ssize_t) GetPixelIndex(image,p)),q);
        pixel=ScaleQuantumToShort(GetPixelAlpha(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
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
            float
              float_pixel;

            q=PopQuantumFloatPixel(quantum_info,(float) GetPixelIndex(image,p),q);
            float_pixel=(float) GetPixelAlpha(image,p);
            q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopLongPixel(quantum_info->endian,(unsigned int)
          GetPixelIndex(image,p),q);
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