// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 31/59


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
            SetPixelRed(image,ScaleLongToQuantum(*p),q);
            break;
          }
          case GreenQuantum:
          case MagentaQuantum:
          {
            SetPixelGreen(image,ScaleLongToQuantum(*p),q);
            break;
          }
          case BlueQuantum:
          case YellowQuantum:
          {
            SetPixelBlue(image,ScaleLongToQuantum(*p),q);
            break;
          }
          case AlphaQuantum:
          {
            SetPixelAlpha(image,ScaleLongToQuantum(*p),q);
            break;
          }
          case OpacityQuantum:
          {
            SetPixelAlpha(image,ScaleLongToQuantum(*p),q);
            break;
          }
          case BlackQuantum:
          {
            SetPixelBlack(image,ScaleLongToQuantum(*p),q);
            break;
          }
          case IndexQuantum:
          {
            SetPixelGray(image,ScaleLongToQuantum(*p),q);
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

static MagickBooleanType ImportLongLongPixel(Image *image,
  const RectangleInfo *roi,const char *magick_restrict map,
  const QuantumType *quantum_map,const void *pixels,ExceptionInfo *exception)
{
  const MagickSizeType
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  size_t
    length;

  ssize_t
    y;