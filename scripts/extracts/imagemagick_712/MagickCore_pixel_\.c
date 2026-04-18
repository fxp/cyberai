// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 28/59



  p=(const float *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    {
      for (y=0; y < (ssize_t) roi->height; y++)
      {
        q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
        if (q == (Quantum *) NULL)
          break;
        for (x=0; x < (ssize_t) roi->width; x++)
        {
          SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        if (SyncAuthenticPixels(image,exception) == MagickFalse)
          break;
      }
      return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
    }
  if (LocaleCompare(map,"BGRA") == 0)
    {
      for (y=0; y < (ssize_t) roi->height; y++)
      {
        q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
        if (q == (Quantum *) NULL)
          break;
        for (x=0; x < (ssize_t) roi->width; x++)
        {
          SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        if (SyncAuthenticPixels(image,exception) == MagickFalse)
          break;
      }
      return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
    }
  if (LocaleCompare(map,"BGRP") == 0)
    {
      for (y=0; y < (ssize_t) roi->height; y++)
      {
        q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
        if (q == (Quantum *) NULL)
          break;
        for (x=0; x < (ssize_t) roi->width; x++)
        {
          SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          p++;
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        if (SyncAuthenticPixels(image,exception) == MagickFalse)
          break;
      }
      return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
    }
  if (LocaleCompare(map,"I") == 0)
    {
      for (y=0; y < (ssize_t) roi->height; y++)
      {
        q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
        if (q == (Quantum *) NULL)
          break;
        for (x=0; x < (ssize_t) roi->width; x++)
        {
          SetPixelGray(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        if (SyncAuthenticPixels(image,exception) == MagickFalse)
          break;
      }
      return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
    }
  if (LocaleCompare(map,"RGB") == 0)
    {
      for (y=0; y < (ssize_t) roi->height; y++)
      {
        q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
        if (q == (Quantum *) NULL)
          break;
        for (x=0; x < (ssize_t) roi->width; x++)
        {
          SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        if (SyncAuthenticPixels(image,exception) == MagickFalse)
          break;
      }
      return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
    }
  if (LocaleCompare(map,"RGBA") == 0)
    {
      for (y=0; y < (ssize_t) roi->height; y++)
      {
        q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
        if (q == (Quantum *) NULL)
          break;
        for (x=0; x < (ssize_t) roi->width; x++)
        {
          SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(double)
            (*p)),q);
          p++;
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        if (SyncAuthenticPixels(image,ex