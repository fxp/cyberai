// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 20/33



          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushDoublePixel(quantum_info,p,&pixel);
            SetPixelGray(image,ClampToQuantum(pixel),q);
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
        SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
  }
}

static void ImportIndexQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const unsigned char *magick_restrict p,
  Quantum *magick_restrict q,ExceptionInfo *exception)
{
  MagickBooleanType
    range_exception;

  ssize_t
    x;

  ssize_t
    bit;

  if (image->storage_class != PseudoClass)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColormappedImageRequired","`%s'",image->filename);
      return;
    }
  range_exception=MagickFalse;
  switch (quantum_info->depth)
  {
    case 1:
    {
      unsigned char
        pixel;

      for (x=0; x < ((ssize_t) number_pixels-7); x+=8)
      {
        for (bit=0; bit < 8; bit++)
        {
          if (quantum_info->min_is_white == MagickFalse)
            pixel=(unsigned char) (((*p) & (1 << (7-bit))) == 0 ?
              0x00 : 0x01);
          else
            pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ?
              0x00 : 0x01);
          SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),
            q);
          SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
            GetPixelIndex(image,q),q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        p++;
      }
      for (bit=0; bit < (ssize_t) (number_pixels % 8); bit++)
      {
        if (quantum_info->min_is_white == MagickFalse)
          pixel=(unsigned char) (((*p) & (1 << (7-bit))) == 0 ? 0x00 : 0x01);
        else
          pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x00 : 0x01);
        SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
        SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
          GetPixelIndex(image,q),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 4:
    {
      unsigned char
        pixel;

      for (x=0; x < ((ssize_t) number_pixels-1); x+=2)
      {
        pixel=(unsigned char) ((*p >> 4) & 0xf);
        SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
        SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
          GetPixelIndex(image,q),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
        pixel=(unsigned char) ((*p) & 0xf);
        SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
        SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
          GetPixelIndex(image,q),q);
        p++;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      for (bit=0; bit < (ssize_t) (number_pixels % 2); bit++)
      {
        pixel=(unsigned char) ((*p++ >> 4) & 0xf);
        SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
        SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
          GetPixelIndex(image,q),q);
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
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 16:
    {
      unsigned short
        pixel;