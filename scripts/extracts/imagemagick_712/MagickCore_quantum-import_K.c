// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 11/33



          n=0;
          quantum=0;
          for (x=0; x < ((ssize_t) number_pixels-3); x+=4)
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
              cbcr[i]=(Quantum) (quantum);
              n++;
            }
            p+=(ptrdiff_t) quantum_info->pad;
            SetPixelRed(image,cbcr[1],q);
            SetPixelGreen(image,cbcr[0],q);
            SetPixelBlue(image,cbcr[2],q);
            q+=(ptrdiff_t) GetPixelChannels(image);
            SetPixelRed(image,cbcr[3],q);
            SetPixelGreen(image,cbcr[0],q);
            SetPixelBlue(image,cbcr[2],q);
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
  }
}

static void ImportCMYKQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const unsigned char *magick_restrict p,
  Quantum *magick_restrict q,ExceptionInfo *exception)
{
  QuantumAny
    range;

  ssize_t
    x;

  if (image->colorspace != CMYKColorspace)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColorSeparatedImageRequired","`%s'",image->filename);
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
        SetPixelRed(image,ScaleCharToQuantum(pixel),q);
        p=PushCharPixel(p,&pixel);
        SetPixelGreen(image,ScaleCharToQuantum(pixel),q);
        p=PushCharPixel(p,&pixel);
        SetPixelBlue(image,ScaleCharToQuantum(pixel),q);
        p=PushCharPixel(p,&pixel);
        SetPixelBlack(image,ScaleCharToQuantum(pixel),q);
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
            SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),q);
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),q);
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
              HalfToSinglePrecision(pixel)),q);
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelBlack(image,ClampToQuantum((double) QuantumRange*(double)
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
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelBlack(image,ScaleShortToQuantum(pixel),q);
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