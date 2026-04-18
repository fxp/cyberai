// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-export.c
// Segment 11/30



      for (x=0; x < (ssize_t) number_pixels; x+=2)
      {
        for (i=0; i < 4; i++)
        {
          switch (n % 3)
          {
            case 0:
            {
              quantum=(size_t) GetPixelRed(image,p);
              break;
            }
            case 1:
            {
              quantum=(size_t) GetPixelGreen(image,p);
              break;
            }
            case 2:
            {
              quantum=(size_t) GetPixelBlue(image,p);
              break;
            }
          }
          cbcr[i]=(Quantum) quantum;
          n++;
        }
        range=GetQuantumRange(quantum_info->depth);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(cbcr[1],range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(cbcr[0],range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(cbcr[2],range),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(cbcr[3],range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(cbcr[0],range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(cbcr[2],range),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
  }
}

static void ExportCMYKQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const Quantum *magick_restrict p,
  unsigned char *magick_restrict q,ExceptionInfo *exception)
{
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
        pixel=ScaleQuantumToChar(GetPixelRed(image,p));
        q=PopCharPixel(pixel,q);
        pixel=ScaleQuantumToChar(GetPixelGreen(image,p));
        q=PopCharPixel(pixel,q);
        pixel=ScaleQuantumToChar(GetPixelBlue(image,p));
        q=PopCharPixel(pixel,q);
        pixel=ScaleQuantumToChar(GetPixelBlack(image,p));
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
            pixel=SinglePrecisionToHalf(QuantumScale*(double)
              GetPixelRed(image,p));
            q=PopShortPixel(quantum_info->endian,pixel,q);
            pixel=SinglePrecisionToHalf(QuantumScale*(double)
              GetPixelGreen(image,p));
            q=PopShortPixel(quantum_info->endian,pixel,q);
            pixel=SinglePrecisionToHalf(QuantumScale*(double)
              GetPixelBlue(image,p));
            q=PopShortPixel(quantum_info->endian,pixel,q);
            pixel=SinglePrecisionToHalf(QuantumScale*(double)
              GetPixelBlack(image,p));
            q=PopShortPixel(quantum_info->endian,pixel,q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=ScaleQuantumToShort(GetPixelRed(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=ScaleQuantumToShort(GetPixelGreen(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=ScaleQuantumToShort(GetPixelBlue(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=ScaleQuantumToShort(GetPixelBlack(image,p));
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