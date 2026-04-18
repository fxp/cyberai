// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 22/33



      for (x=((ssize_t) number_pixels-3); x > 0; x-=4)
      {
        for (bit=0; bit < 8; bit+=2)
        {
          if (quantum_info->min_is_white == MagickFalse)
            pixel=(unsigned char) (((*p) & (1 << (7-bit))) == 0 ? 0x00 : 0x01);
          else
            pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x00 : 0x01);
          SetPixelGray(image,(Quantum) (pixel == 0 ? 0 : QuantumRange),q);
          SetPixelAlpha(image,((*p) & (1UL << (unsigned char) (6-bit))) == 0 ?
            TransparentAlpha : OpaqueAlpha,q);
          SetPixelIndex(image,(Quantum) (pixel == 0 ? 0 : 1),q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
      }
      if ((number_pixels % 4) != 0)
        for (bit=3; bit >= (ssize_t) (4-(number_pixels % 4)); bit-=2)
        {
          if (quantum_info->min_is_white == MagickFalse)
            pixel=(unsigned char) (((*p) & (1 << (7-bit))) == 0 ? 0x00 : 0x01);
          else
            pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x00 : 0x01);
          SetPixelIndex(image,(Quantum) (pixel == 0 ? 0 : 1),q);
          SetPixelGray(image,(Quantum) (pixel == 0 ? 0 : QuantumRange),q);
          SetPixelAlpha(image,((*p) & (1UL << (unsigned char) (6-bit))) == 0 ?
            TransparentAlpha : OpaqueAlpha,q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
      break;
    }
    case 4:
    {
      unsigned char
        pixel;

      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=(unsigned char) ((*p >> 4) & 0xf);
        SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
        SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
          GetPixelIndex(image,q),q);
        pixel=(unsigned char) ((*p) & 0xf);
        SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
        p++;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 8:
    {
      unsigned char
        pixel;

      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushCharPixel(p,&pixel);
        SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
        SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
          GetPixelIndex(image,q),q);
        p=PushCharPixel(p,&pixel);
        SetPixelAlpha(image,ScaleCharToQuantum(pixel),q);
        p+=(ptrdiff_t) quantum_info->pad;
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
            SetPixelIndex(image,PushColormapIndex(image,(size_t)
              ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),&range_exception),q);
            SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
              GetPixelIndex(image,q),q);
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
        SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
          GetPixelIndex(image,q),q);
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelAlpha(image,ScaleShortToQuantum(pixel),q);
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
            SetPixelIndex(image,PushColormapIndex(image,(size_t)
              ClampToQuantum(pixel),&range_exception),q);
            SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
              GetPixelIndex(image,q),q);
            p=PushQuantumFloatPixel(quantum_info,p,&pixel);
            SetPixelAlpha(image,ClampToQuantum(pixel),q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      else
        {
          unsigned int
            pixel;