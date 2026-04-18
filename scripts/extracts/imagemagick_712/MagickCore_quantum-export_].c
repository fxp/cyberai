// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-export.c
// Segment 29/30



            q=PopQuantumFloatPixel(quantum_info,(float) GetPixelRed(image,p),
              q);
            q=PopQuantumFloatPixel(quantum_info,(float) GetPixelGreen(image,p),
              q);
            q=PopQuantumFloatPixel(quantum_info,(float) GetPixelBlue(image,p),
              q);
            float_pixel=(float) GetPixelOpacity(image,p);
            q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=ScaleQuantumToLong(GetPixelRed(image,p));
        q=PopLongPixel(quantum_info->endian,pixel,q);
        pixel=ScaleQuantumToLong(GetPixelGreen(image,p));
        q=PopLongPixel(quantum_info->endian,pixel,q);
        pixel=ScaleQuantumToLong(GetPixelBlue(image,p));
        q=PopLongPixel(quantum_info->endian,pixel,q);
        pixel=ScaleQuantumToLong(GetPixelOpacity(image,p));
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
          double
            pixel;

          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            q=PopQuantumDoublePixel(quantum_info,(double)
              GetPixelRed(image,p),q);
            q=PopQuantumDoublePixel(quantum_info,(double)
              GetPixelGreen(image,p),q);
            q=PopQuantumDoublePixel(quantum_info,(double)
              GetPixelBlue(image,p),q);
            pixel=(double) GetPixelOpacity(image,p);
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
      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelRed(image,p),
          range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelGreen(image,p),
          range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlue(image,p),
          range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelOpacity(image,p),
          range),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
  }
}

MagickExport size_t ExportQuantumPixels(const Image *image,
  CacheView *image_view,QuantumInfo *quantum_info,
  const QuantumType quantum_type,unsigned char *magick_restrict pixels,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  MagickSizeType
    number_pixels;

  size_t
    extent;

  ssize_t
    x;

  unsigned char
    *magick_restrict q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(quantum_info != (QuantumInfo *) NULL);
  assert(quantum_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (pixels == (unsigned char *) NULL)
    pixels=(unsigned char *) GetQuantumPixels(quantum_info);
  if (image_view == (CacheView *) NULL)
    {
      number_pixels=GetImageExtent(image);
      p=GetVirtualPixelQueue(image);
    }
  else
    {
      number_pixels=GetCacheViewExtent(image_view);
      p=GetCacheViewVirtualPixelQueue(image_view);
    }
  if (p == (Quantum *) NULL)
    return(0);
  if (quantum_info->alpha_type == AssociatedQuantumAlpha)
    {
      double
        Sa;

      Quantum
        *magick_restrict r;

      /*
        Associate alpha.
      */
      if (image_view == (CacheView *) NULL)
        r=GetAuthenticPixelQueue(image);
      else
        r=GetCacheViewAuthenticPixelQueue(image_view);
      for (x=0; x < (ssize_t) image->columns; x++)
      {
        ssize_t
          i;

        Sa=QuantumScale*(double) GetPixelAlpha(image,r);
        for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
        {
          PixelChannel channel = GetPixelChannelChannel(image,i);
          PixelTrait traits = GetPixelChannelTraits(image,channel);
          if ((traits & UpdatePixelTrait) == 0)
            continue;
          r[i]=ClampToQuantum(Sa*(double) r[i]);
        }
        r+=(ptrdiff_t) GetPixelChannels(image);
      }
    }
  if ((quantum_type == CbYCrQuantum) || (quantum_type == CbYCrAQuantum))
    {
      Quantum
        quantum;

      Quantum
        *magick_restrict r;