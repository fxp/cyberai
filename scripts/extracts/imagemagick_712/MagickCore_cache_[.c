// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 27/53



        /*
          Pixel request is inside cache extents.
        */
        if (nexus_info->authentic_pixel_cache != MagickFalse)
          return(pixels);
        status=ReadPixelCachePixels(cache_info,nexus_info,exception);
        if (status == MagickFalse)
          return((const Quantum *) NULL);
        if (cache_info->metacontent_extent != 0)
          {
            status=ReadPixelCacheMetacontent(cache_info,nexus_info,exception);
            if (status == MagickFalse)
              return((const Quantum *) NULL);
          }
        return(pixels);
      }
  /*
    Pixel request is outside cache extents.
  */
  virtual_nexus=nexus_info->virtual_nexus;
  q=pixels;
  s=(unsigned char *) nexus_info->metacontent;
  (void) memset(virtual_pixel,0,cache_info->number_channels*
    sizeof(*virtual_pixel));
  virtual_metacontent=(void *) NULL;
  switch (virtual_pixel_method)
  {
    case BackgroundVirtualPixelMethod:
    case BlackVirtualPixelMethod:
    case GrayVirtualPixelMethod:
    case TransparentVirtualPixelMethod:
    case MaskVirtualPixelMethod:
    case WhiteVirtualPixelMethod:
    case EdgeVirtualPixelMethod:
    case CheckerTileVirtualPixelMethod:
    case HorizontalTileVirtualPixelMethod:
    case VerticalTileVirtualPixelMethod:
    {
      if (cache_info->metacontent_extent != 0)
        {
          /*
            Acquire a metacontent buffer.
          */
          virtual_metacontent=(void *) AcquireQuantumMemory(1,
            cache_info->metacontent_extent);
          if (virtual_metacontent == (void *) NULL)
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                CacheError,"UnableToGetCacheNexus","`%s'",image->filename);
              return((const Quantum *) NULL);
            }
          (void) memset(virtual_metacontent,0,cache_info->metacontent_extent);
        }
      switch (virtual_pixel_method)
      {
        case BlackVirtualPixelMethod:
        {
          for (i=0; i < (ssize_t) cache_info->number_channels; i++)
            SetPixelChannel(image,(PixelChannel) i,(Quantum) 0,virtual_pixel);
          SetPixelAlpha(image,OpaqueAlpha,virtual_pixel);
          break;
        }
        case GrayVirtualPixelMethod:
        {
          for (i=0; i < (ssize_t) cache_info->number_channels; i++)
            SetPixelChannel(image,(PixelChannel) i,QuantumRange/2,
              virtual_pixel);
          SetPixelAlpha(image,OpaqueAlpha,virtual_pixel);
          break;
        }
        case TransparentVirtualPixelMethod:
        {
          for (i=0; i < (ssize_t) cache_info->number_channels; i++)
            SetPixelChannel(image,(PixelChannel) i,(Quantum) 0,virtual_pixel);
          SetPixelAlpha(image,TransparentAlpha,virtual_pixel);
          break;
        }
        case MaskVirtualPixelMethod:
        case WhiteVirtualPixelMethod:
        {
          for (i=0; i < (ssize_t) cache_info->number_channels; i++)
            SetPixelChannel(image,(PixelChannel) i,QuantumRange,virtual_pixel);
          SetPixelAlpha(image,OpaqueAlpha,virtual_pixel);
          break;
        }
        default:
        {
          SetPixelRed(image,ClampToQuantum(image->background_color.red),
            virtual_pixel);
          SetPixelGreen(image,ClampToQuantum(image->background_color.green),
            virtual_pixel);
          SetPixelBlue(image,ClampToQuantum(image->background_color.blue),
            virtual_pixel);
          SetPixelBlack(image,ClampToQuantum(image->background_color.black),
            virtual_pixel);
          SetPixelAlpha(image,ClampToQuantum(image->background_color.alpha),
            virtual_pixel);
          break;
        }
      }
      break;
    }
    default:
      break;
  }
  for (v=0; v < (ssize_t) rows; v++)
  {
    ssize_t
      y_offset;

    y_offset=y+v;
    if ((virtual_pixel_method == EdgeVirtualPixelMethod) ||
        (virtual_pixel_method == UndefinedVirtualPixelMethod))
      y_offset=EdgeY(y_offset,cache_info->rows);
    for (u=0; u < (ssize_t) columns; u+=(ssize_t) length)
    {
      ssize_t
        x_offset;

      x_offset=x+u;
      length=(MagickSizeType) MagickMin((ssize_t) cache_info->columns-
        x_offset,(ssize_t) columns-u);
      if (((x_offset < 0) || (x_offset >= (ssize_t) cache_info->columns)) ||
          ((y_offset < 0) || (y_offset >= (ssize_t) cache_info->rows)) ||
          (length == 0))
        {
          MagickModulo
            x_modulo,
            y_modulo;