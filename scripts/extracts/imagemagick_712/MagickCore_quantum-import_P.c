// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 16/33



          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushDoublePixel(quantum_info,p,&pixel);
            SetPixelRed(image,ClampToQuantum(pixel),q);
            p=PushDoublePixel(quantum_info,p,&pixel);
            SetPixelGreen(image,ClampToQuantum(pixel),q);
            p=PushDoublePixel(quantum_info,p,&pixel);
            SetPixelBlue(image,ClampToQuantum(pixel),q);
            p=PushDoublePixel(quantum_info,p,&pixel);
            SetPixelBlack(image,ClampToQuantum(pixel),q);
            p=PushDoublePixel(quantum_info,p,&pixel);
            SetPixelOpacity(image,ClampToQuantum(pixel),q);
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
        pixel;

      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelBlack(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelOpacity(image,ScaleAnyToQuantum(pixel,range),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
  }
}

static void ImportGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const unsigned char *magick_restrict p,
  Quantum *magick_restrict q)
{
  QuantumAny
    range;

  ssize_t
    x;

  ssize_t
    bit;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 1:
    {
      Quantum
        black,
        white;

      black=(Quantum) 0;
      white=QuantumRange;
      if (quantum_info->min_is_white != MagickFalse)
        {
          black=QuantumRange;
          white=(Quantum) 0;
        }
      for (x=0; x < ((ssize_t) number_pixels-7); x+=8)
      {
        for (bit=0; bit < 8; bit++)
        {
          SetPixelGray(image,((*p) & (1 << (7-bit))) == 0 ? black : white,q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        p++;
      }
      for (bit=0; bit < (ssize_t) (number_pixels % 8); bit++)
      {
        SetPixelGray(image,((*p) & (0x01 << (7-bit))) == 0 ? black : white,q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      if (bit != 0)
        p++;
      break;
    }
    case 4:
    {
      unsigned char
        pixel;

      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < ((ssize_t) number_pixels-1); x+=2)
      {
        pixel=(unsigned char) ((*p >> 4) & 0xf);
        SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((*p) & 0xf);
        SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
        p++;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      for (bit=0; bit < (ssize_t) (number_pixels % 2); bit++)
      {
        pixel=(unsigned char) (*p++ >> 4);
        SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 8:
    {
      unsigned char
        pixel;

      if (quantum_info->min_is_white != MagickFalse)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushCharPixel(p,&pixel);
            SetPixelGray(image,QuantumRange-ScaleCharToQuantum(pixel),q);
            SetPixelAlpha(image,OpaqueAlpha,q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushCharPixel(p,&pixel);
        SetPixelGray(image,ScaleCharToQuantum(pixel),q);
        SetPixelAlpha(image,OpaqueAlpha,q);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 10:
    {
      unsigned int
        pixel;