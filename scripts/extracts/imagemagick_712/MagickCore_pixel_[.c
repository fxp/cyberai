// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 27/59

ize_t) roi->width; x++)
        {
          SetPixelRed(image,ClampToQuantum((double) QuantumRange*(*p)),q);
          p++;
          SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(*p)),q);
          p++;
          SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(*p)),q);
          p++;
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        if (SyncAuthenticPixels(image,exception) == MagickFalse)
          break;
      }
      return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
    }
   length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
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
            SetPixelRed(image,ClampToQuantum((double) QuantumRange*(*p)),q);
            break;
          }
          case GreenQuantum:
          case MagentaQuantum:
          {
            SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(*p)),q);
            break;
          }
          case BlueQuantum:
          case YellowQuantum:
          {
            SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(*p)),q);
            break;
          }
          case AlphaQuantum:
          {
            SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(*p)),q);
            break;
          }
          case OpacityQuantum:
          {
            SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(*p)),q);
            break;
          }
          case BlackQuantum:
          {
            SetPixelBlack(image,ClampToQuantum((double) QuantumRange*(*p)),q);
            break;
          }
          case IndexQuantum:
          {
            SetPixelGray(image,ClampToQuantum((double) QuantumRange*(*p)),q);
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

static MagickBooleanType ImportFloatPixel(Image *image,const RectangleInfo *roi,
  const char *magick_restrict map,const QuantumType *quantum_map,
  const void *pixels,ExceptionInfo *exception)
{
  const float
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  size_t
    length;

  ssize_t
    y;