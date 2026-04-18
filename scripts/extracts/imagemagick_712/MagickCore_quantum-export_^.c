// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-export.c
// Segment 30/30



      if (image_view == (CacheView *) NULL)
        r=GetAuthenticPixelQueue(image);
      else
        r=GetCacheViewAuthenticPixelQueue(image_view);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        quantum=GetPixelRed(image,r);
        SetPixelRed(image,GetPixelGreen(image,r),r);
        SetPixelGreen(image,quantum,r);
        r+=(ptrdiff_t) GetPixelChannels(image);
      }
    }
  q=pixels;
  ResetQuantumState(quantum_info);
  extent=GetQuantumExtent(image,quantum_info,quantum_type);
  switch (quantum_type)
  {
    case AlphaQuantum:
    {
      ExportAlphaQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case BGRQuantum:
    {
      ExportBGRQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case BGRAQuantum:
    {
      ExportBGRAQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case BGROQuantum:
    {
      ExportBGROQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case BlackQuantum:
    {
      ExportBlackQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      ExportPixelChannel(image,quantum_info,number_pixels,p,q,BluePixelChannel);
      break;
    }
    case CMYKQuantum:
    {
      ExportCMYKQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case CMYKAQuantum:
    {
      ExportCMYKAQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case MultispectralQuantum:
    {
      ExportMultispectralQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case CMYKOQuantum:
    {
      ExportCMYKOQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case CbYCrYQuantum:
    {
      ExportCbYCrYQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case GrayQuantum:
    {
      ExportGrayQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case GrayAlphaQuantum:
    {
      ExportGrayAlphaQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      ExportPixelChannel(image,quantum_info,number_pixels,p,q,
        GreenPixelChannel);
      break;
    }
    case IndexQuantum:
    {
      ExportIndexQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case IndexAlphaQuantum:
    {
      ExportIndexAlphaQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case RedQuantum:
    case CyanQuantum:
    {
      ExportPixelChannel(image,quantum_info,number_pixels,p,q,RedPixelChannel);
      break;
    }
    case OpacityQuantum:
    {
      ExportOpacityQuantum(image,quantum_info,number_pixels,p,q,exception);
      break;
    }
    case RGBQuantum:
    case CbYCrQuantum:
    {
      ExportRGBQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case RGBAQuantum:
    case CbYCrAQuantum:
    {
      ExportRGBAQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    case RGBOQuantum:
    {
      ExportRGBOQuantum(image,quantum_info,number_pixels,p,q);
      break;
    }
    default:
      break;
  }
  if ((quantum_type == CbYCrQuantum) || (quantum_type == CbYCrAQuantum))
    {
      Quantum
        quantum;

      Quantum
        *magick_restrict r;

      if (image_view == (CacheView *) NULL)
        r=GetAuthenticPixelQueue(image);
      else
        r=GetCacheViewAuthenticPixelQueue(image_view);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        quantum=GetPixelRed(image,r);
        SetPixelRed(image,GetPixelGreen(image,r),r);
        SetPixelGreen(image,quantum,r);
        r+=(ptrdiff_t) GetPixelChannels(image);
      }
    }
  return(extent);
}
