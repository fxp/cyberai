// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 32/33



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
        SetPixelOpacity(image,ScaleAnyToQuantum(pixel,range),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
  }
}

MagickExport size_t ImportQuantumPixels(const Image *image,
  CacheView *image_view,QuantumInfo *quantum_info,
  const QuantumType quantum_type,const unsigned char *magick_restrict pixels,
  ExceptionInfo *exception)
{
  const unsigned char
    *magick_restrict p;

  MagickSizeType
    number_pixels;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  size_t
    extent;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(quantum_info != (QuantumInfo *) NULL);
  assert(quantum_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (pixels == (const unsigned char *) NULL)
    pixels=(const unsigned char *) GetQuantumPixels(quantum_info);
  x=0;
  p=pixels;
  if (image_view == (CacheView *) NULL)
    {
      number_pixels=GetImageExtent(image);
      q=GetAuthenticPixelQueue(image);
    }
  else
    {
      number_pixels=GetCacheViewExtent(image_view);
      q=GetCacheViewAuthenticPixelQueue(image_view);
    }
  if (q == (Quantum *) NULL)
    return(0);
  ResetQuantumState(quantum_info);
  extent=GetQuantumExtent(image,quantum_info,quantum_type);
  switch (quantum_type)
  {
    case AlphaQuantum:
    {
      ImportAlphaQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case BGRQuantum:
    {
      ImportBGRQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case BGRAQuantum:
    {
      ImportBGRAQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case BGROQuantum:
    {
      ImportBGROQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case BlackQuantum:
    {
      ImportBlackQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      ImportPixelChannel(image,quantum_info,number_pixels,p,q,BluePixelChannel);
      break;
    }
    case CMYKQuantum:
    {
      ImportCMYKQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case CMYKAQuantum:
    {
      ImportCMYKAQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case MultispectralQuantum:
    {
      ImportMultispectralQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case CMYKOQuantum:
    {
      ImportCMYKOQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case CbYCrYQuantum:
    {
      ImportCbYCrYQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case GrayQuantum:
    {
      ImportGrayQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case GrayAlphaQuantum:
    {
      ImportGrayAlphaQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      ImportPixelChannel(image,quantum_info,number_pixels,p,q,GreenPixelChannel);
      break;
    }
    case IndexQuantum:
    {
      ImportIndexQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case IndexAlphaQuantum:
    {
      ImportIndexAlphaQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case OpacityQuantum:
    {
      ImportOpacityQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case RedQuantum:
    case CyanQuantum:
    {
      ImportPixelChannel(image,quantum_info,number_pixels,p,q,RedPixelChannel);
      break;
    }
    case RGBQuantum:
    case CbYCrQuantum:
    {
      ImportRGBQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case RGBAQuantum:
    case CbYCrAQuantum:
    {
      ImportRGBAQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case RGBOQuantum:
    {
      ImportRGBOQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    default:
      break;
  }
  if ((quantum_type == CbYCrQuantum) || (quantum_type == CbYCrAQuantum))
    {
      Quantum
        quantum;