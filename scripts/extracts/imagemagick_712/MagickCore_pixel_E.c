// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 37/59

oi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      ssize_t
        i;

      for (i=0; i < (ssize_t) length; i++)
      {
        switch (quantum_map[i])
        {
          case RedQuantum:
          case CyanQuantum:
          {
            SetPixelRed(image,ScaleShortToQuantum(*p),q);
            break;
          }
          case GreenQuantum:
          case MagentaQuantum:
          {
            SetPixelGreen(image,ScaleShortToQuantum(*p),q);
            break;
          }
          case BlueQuantum:
          case YellowQuantum:
          {
            SetPixelBlue(image,ScaleShortToQuantum(*p),q);
            break;
          }
          case AlphaQuantum:
          {
            SetPixelAlpha(image,ScaleShortToQuantum(*p),q);
            break;
          }
          case OpacityQuantum:
          {
            SetPixelAlpha(image,ScaleShortToQuantum(*p),q);
            break;
          }
          case BlackQuantum:
          {
            SetPixelBlack(image,ScaleShortToQuantum(*p),q);
            break;
          }
          case IndexQuantum:
          {
            SetPixelGray(image,ScaleShortToQuantum(*p),q);
            break;
          }
          default:
            break;
        }
        p++;
      }
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

MagickExport MagickBooleanType ImportImagePixels(Image *image,const ssize_t x,
  const ssize_t y,const size_t width,const size_t height,const char *map,
  const StorageType type,const void *pixels,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  QuantumType
    *quantum_map;

  RectangleInfo
    roi;

  ssize_t
    i;

  size_t
    length;