// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-export.c
// Segment 15/30



          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            q=PopQuantumDoublePixel(quantum_info,(double)
              GetPixelRed(image,p),q);
            q=PopQuantumDoublePixel(quantum_info,(double)
              GetPixelGreen(image,p),q);
            q=PopQuantumDoublePixel(quantum_info,(double)
              GetPixelBlue(image,p),q);
            q=PopQuantumDoublePixel(quantum_info,(double)
              GetPixelBlack(image,p),q);
            pixel=(double) (GetPixelOpacity(image,p));
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
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelRed(image,p),
          range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelGreen(image,p),
          range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlue(image,p),
          range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlack(image,p),
          range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(
          GetPixelOpacity(image,p),range),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
  }
}

static void ExportGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const Quantum *magick_restrict p,
  unsigned char *magick_restrict q)
{
  QuantumAny
    range;

  ssize_t
    x;

  switch (quantum_info->depth)
  {
    case 1:
    {
      double
        threshold;

      unsigned char
        black,
        white;

      ssize_t
        bit;

      black=0x00;
      white=0x01;
      if (quantum_info->min_is_white != MagickFalse)
        {
          black=0x01;
          white=0x00;
        }
      threshold=(double) QuantumRange/2.0;
      for (x=((ssize_t) number_pixels-7); x > 0; x-=8)
      {
        *q='\0';
        *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 7;
        p+=(ptrdiff_t) GetPixelChannels(image);
        *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 6;
        p+=(ptrdiff_t) GetPixelChannels(image);
        *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 5;
        p+=(ptrdiff_t) GetPixelChannels(image);
        *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 4;
        p+=(ptrdiff_t) GetPixelChannels(image);
        *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 3;
        p+=(ptrdiff_t) GetPixelChannels(image);
        *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 2;
        p+=(ptrdiff_t) GetPixelChannels(image);
        *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 1;
        p+=(ptrdiff_t) GetPixelChannels(image);
        *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 0;
        p+=(ptrdiff_t) GetPixelChannels(image);
        q++;
      }
      if ((number_pixels % 8) != 0)
        {
          *q='\0';
          for (bit=7; bit >= (ssize_t) (8-(number_pixels % 8)); bit--)
          {
            *q|=(GetPixelLuma(image,p) < threshold ? black : white) << bit;
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

      for (x=0; x < ((ssize_t) number_pixels-1) ; x+=2)
      {
        pixel=ScaleQuantumToChar(ClampToQuantum(GetPixelLuma(image,p)));
        *q=(((pixel >> 4) & 0xf) << 4);
        p+=(ptrdiff_t) GetPixelChannels(image);
        pixel=ScaleQuantumToChar(ClampToQuantum(GetPixelLuma(image,p)));
        *q|=pixel >> 4;
        p+=(ptrdiff_t) GetPixelChannels(image);
        q++;
      }
      if ((number_pixels % 2) != 0)
        {
          pixel=ScaleQuantumToChar(ClampToQuantum(GetPixelLuma(image,p)));
          *q=(((pixel >> 4) & 0xf) << 4);
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
        pixel=ScaleQuantumToChar(ClampToQuantum(GetPixelLuma(image,p)));
        q=PopCharPixel(pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
    case 10:
    {
      range=GetQuantumRange(quantum_info->depth);
      if (quantum_info->pack == MagickFalse)
        {
          unsigned int
            pixel;