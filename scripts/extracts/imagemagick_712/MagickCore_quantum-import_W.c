// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 23/33



          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            SetPixelIndex(image,PushColormapIndex(image,pixel,
              &range_exception),q);
            SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
              GetPixelIndex(image,q),q);
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
            SetPixelIndex(image,PushColormapIndex(image,(size_t)
              ClampToQuantum(pixel),&range_exception),q);
            SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
              GetPixelIndex(image,q),q);
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
            SetPixelIndex(image,PushColormapIndex(image,(size_t)
              ClampToQuantum(pixel),&range_exception),q);
            SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
              GetPixelIndex(image,q),q);
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
        SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
        SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
          GetPixelIndex(image,q),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
  }
  if (range_exception != MagickFalse)
    (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
      "InvalidColormapIndex","`%s'",image->filename);
}

static void ImportMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const MagickSizeType number_pixels,
  const unsigned char *magick_restrict p,Quantum *magick_restrict q,
  ExceptionInfo *exception)
{
  QuantumAny
    range;

  ssize_t
    i,
    x;

  if (image->number_meta_channels == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "MultispectralImageRequired","`%s'",image->filename);
      return;
    }
  if (quantum_info->meta_channel != 0)
    {
      ImportPixelChannel(image,quantum_info,number_pixels,p,q,
        (PixelChannel) (MetaPixelChannels+quantum_info->meta_channel-1));
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
        for (i=0; i < (ssize_t) GetImageChannels(image); i++)
        {
          p=PushCharPixel(p,&pixel);
          q[i]=ScaleCharToQuantum(pixel);
        }
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 16:
    {
      unsigned short
        pixel;