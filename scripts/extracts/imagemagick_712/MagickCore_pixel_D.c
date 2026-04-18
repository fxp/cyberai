// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 36/59



  p=(const unsigned short *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    {
      for (y=0; y < (ssize_t) roi->height; y++)
      {
        q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
        if (q == (Quantum *) NULL)
          break;
        for (x=0; x < (ssize_t) roi->width; x++)
        {
          SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
          SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
          SetPixelRed(image,ScaleShortToQuantum(*p++),q);
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
          SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
          SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
          SetPixelRed(image,ScaleShortToQuantum(*p++),q);
          SetPixelAlpha(image,ScaleShortToQuantum(*p++),q);
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
          SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
          SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
          SetPixelRed(image,ScaleShortToQuantum(*p++),q);
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
          SetPixelGray(image,ScaleShortToQuantum(*p++),q);
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
          SetPixelRed(image,ScaleShortToQuantum(*p++),q);
          SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
          SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
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
          SetPixelRed(image,ScaleShortToQuantum(*p++),q);
          SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
          SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
          SetPixelAlpha(image,ScaleShortToQuantum(*p++),q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        if (SyncAuthenticPixels(image,exception) == MagickFalse)
          break;
      }
      return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
    }
  if (LocaleCompare(map,"RGBP") == 0)
    {
      for (y=0; y < (ssize_t) roi->height; y++)
      {
        q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
        if (q == (Quantum *) NULL)
          break;
        for (x=0; x < (ssize_t) roi->width; x++)
        {
          SetPixelRed(image,ScaleShortToQuantum(*p++),q);
          SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
          SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
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
    q=GetAuthenticPixels(image,roi->x,roi->y+y,r