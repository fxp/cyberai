// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 8/33



          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushQuantumFloatPixel(quantum_info,p,&pixel);
            SetPixelRed(image,ClampToQuantum(pixel),q);
            p=PushQuantumFloatPixel(quantum_info,p,&pixel);
            SetPixelGreen(image,ClampToQuantum(pixel),q);
            p=PushQuantumFloatPixel(quantum_info,p,&pixel);
            SetPixelBlue(image,ClampToQuantum(pixel),q);
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

          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            SetPixelBlue(image,ScaleLongToQuantum(pixel),q);
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            SetPixelGreen(image,ScaleLongToQuantum(pixel),q);
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            SetPixelRed(image,ScaleLongToQuantum(pixel),q);
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            SetPixelAlpha(image,ScaleLongToQuantum(pixel),q);
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
            p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
            SetPixelAlpha(image,ClampToQuantum(pixel),q);
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
            p=PushDoublePixel(quantum_info,p,&pixel);
            SetPixelRed(image,ClampToQuantum(pixel),q);
            p=PushDoublePixel(quantum_info,p,&pixel);
            SetPixelGreen(image,ClampToQuantum(pixel),q);
            p=PushDoublePixel(quantum_info,p,&pixel);
            SetPixelBlue(image,ClampToQuantum(pixel),q);
            p=PushDoublePixel(quantum_info,p,&pixel);
            SetPixelAlpha(image,ClampToQuantum(pixel),q);
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
        SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
  }
}

static void ImportBGROQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const unsigned char *magick_restrict p,
  Quantum *magick_restrict q)
{
  QuantumAny
    range;

  ssize_t
    x;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 8:
    {
      unsigned char
        pixel;

      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushCharPixel(p,&pixel);
        SetPixelBlue(image,ScaleCharToQuantum(pixel),q);
        p=PushCharPixel(p,&pixel);
        SetPixelGreen(image,ScaleCharToQuantum(pixel),q);
        p=PushCharPixel(p,&pixel);
        SetPixelRed(image,ScaleCharToQuantum(pixel),q);
        p=PushCharPixel(p,&pixel);
        SetPixelOpacity(image,ScaleCharToQuantum(pixel),q);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 10:
    {
      unsigned int
        pixel;

      pixel=0;
      if (quantum_info->pack == MagickFalse)
        {
          ssize_t
            i;

          size_t
            quantum;

          ssize_t
            n;