// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 15/18



        /*
          Identify transparent colormap index.
        */
        if ((image->storage_class == DirectClass) || (image->colors > 256))
          (void) SetImageType(image,PaletteBilevelAlphaType,exception);
        for (i=0; i < (ssize_t) image->colors; i++)
          if (image->colormap[i].alpha != (double) OpaqueAlpha)
            {
              if (opacity < 0)
                {
                  opacity=i;
                  continue;
                }
              alpha=fabs(image->colormap[i].alpha-(double) TransparentAlpha);
              beta=fabs(image->colormap[opacity].alpha-(double)
                TransparentAlpha);
              if (alpha < beta)
                opacity=i;
            }
        if (opacity == -1)
          {
            (void) SetImageType(image,PaletteBilevelAlphaType,exception);
            for (i=0; i < (ssize_t) image->colors; i++)
              if (image->colormap[i].alpha != (double) OpaqueAlpha)
                {
                  if (opacity < 0)
                    {
                      opacity=i;
                      continue;
                    }
                  alpha=fabs(image->colormap[i].alpha-(double)
                    TransparentAlpha);
                  beta=fabs(image->colormap[opacity].alpha-(double)
                    TransparentAlpha);
                  if (alpha < beta)
                    opacity=i;
                }
          }
        if (opacity >= 0)
          {
            image->colormap[opacity].red=image->transparent_color.red;
            image->colormap[opacity].green=image->transparent_color.green;
            image->colormap[opacity].blue=image->transparent_color.blue;
          }
      }
    if ((image->storage_class == DirectClass) || (image->colors > 256))
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    for (bits_per_pixel=1; bits_per_pixel < 8; bits_per_pixel++)
      if ((one << bits_per_pixel) >= image->colors)
        break;
    q=colormap;
    for (i=0; i < (ssize_t) image->colors; i++)
    {
      *q++=ScaleQuantumToChar(ClampToQuantum(image->colormap[i].red));
      *q++=ScaleQuantumToChar(ClampToQuantum(image->colormap[i].green));
      *q++=ScaleQuantumToChar(ClampToQuantum(image->colormap[i].blue));
    }
    for ( ; i < (ssize_t) (one << bits_per_pixel); i++)
    {
      *q++=(unsigned char) 0x0;
      *q++=(unsigned char) 0x0;
      *q++=(unsigned char) 0x0;
    }
    if ((GetPreviousImageInList(image) == (Image *) NULL) ||
        (write_info->adjoin == MagickFalse))
      {
        /*
          Write global colormap.
        */
        c=0x80;
        c|=(8-1) << 4;  /* color resolution */
        c|=(int) (bits_per_pixel-1);   /* size of global colormap */
        (void) WriteBlobByte(image,(unsigned char) c);
        for (j=0; j < (ssize_t) image->colors; j++)
          if (IsPixelInfoEquivalent(&image->background_color,image->colormap+j))
            break;
        (void) WriteBlobByte(image,(unsigned char)
          (j == (ssize_t) image->colors ? 0 : j));  /* background color */
        (void) WriteBlobByte(image,(unsigned char) 0x00);  /* reserved */
        length=(size_t) (3*(one << bits_per_pixel));
        (void) WriteBlob(image,length,colormap);
        for (j=0; j < 768; j++)
          global_colormap[j]=colormap[j];
      }
    if (LocaleCompare(write_info->magick,"GIF87") != 0)
      {
        const char
          *value;