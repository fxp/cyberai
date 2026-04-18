// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 28/53



          /*
            Transfer a single pixel.
          */
          length=(MagickSizeType) 1;
          switch (virtual_pixel_method)
          {
            case EdgeVirtualPixelMethod:
            default:
            {
              p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
                EdgeX(x_offset,cache_info->columns),
                EdgeY(y_offset,cache_info->rows),1UL,1UL,virtual_nexus,
                exception);
              r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
              break;
            }
            case RandomVirtualPixelMethod:
            {
              if (cache_info->random_info == (RandomInfo *) NULL)
                cache_info->random_info=AcquireRandomInfo();
              p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
                RandomX(cache_info->random_info,cache_info->columns),
                RandomY(cache_info->random_info,cache_info->rows),1UL,1UL,
                virtual_nexus,exception);
              r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
              break;
            }
            case DitherVirtualPixelMethod:
            {
              p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
                DitherX(x_offset,cache_info->columns),
                DitherY(y_offset,cache_info->rows),1UL,1UL,virtual_nexus,
                exception);
              r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
              break;
            }
            case TileVirtualPixelMethod:
            {
              x_modulo=VirtualPixelModulo(x_offset,cache_info->columns);
              y_modulo=VirtualPixelModulo(y_offset,cache_info->rows);
              p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
                x_modulo.remainder,y_modulo.remainder,1UL,1UL,virtual_nexus,
                exception);
              r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
              break;
            }
            case MirrorVirtualPixelMethod:
            {
              x_modulo=VirtualPixelModulo(x_offset,cache_info->columns);
              if ((x_modulo.quotient & 0x01) == 1L)
                x_modulo.remainder=(ssize_t) cache_info->columns-
                  x_modulo.remainder-1L;
              y_modulo=VirtualPixelModulo(y_offset,cache_info->rows);
              if ((y_modulo.quotient & 0x01) == 1L)
                y_modulo.remainder=(ssize_t) cache_info->rows-
                  y_modulo.remainder-1L;
              p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
                x_modulo.remainder,y_modulo.remainder,1UL,1UL,virtual_nexus,
                exception);
              r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
              break;
            }
            case HorizontalTileEdgeVirtualPixelMethod:
            {
              x_modulo=VirtualPixelModulo(x_offset,cache_info->columns);
              p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
                x_modulo.remainder,EdgeY(y_offset,cache_info->rows),1UL,1UL,
                virtual_nexus,exception);
              r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
              break;
            }
            case VerticalTileEdgeVirtualPixelMethod:
            {
              y_modulo=VirtualPixelModulo(y_offset,cache_info->rows);
              p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
                EdgeX(x_offset,cache_info->columns),y_modulo.remainder,1UL,1UL,
                virtual_nexus,exception);
              r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
              break;
            }
            case BackgroundVirtualPixelMethod:
            case BlackVirtualPixelMethod:
            case GrayVirtualPixelMethod:
            case TransparentVirtualPixelMethod:
            case MaskVirtualPixelMethod:
            case WhiteVirtualPixelMethod:
            {
              p=virtual_pixel;
              r=virtual_metacontent;
              break;
            }
            case CheckerTileVirtualPixelMethod:
            {
              x_modulo=VirtualPixelModulo(x_offset,cache_info->columns);
              y_modulo=VirtualPixelModulo(y_offset,cache_info->rows);
              if (((x_modulo.quotient ^ y_modulo.quotient) & 0x01) != 0L)
                {
                  p=virtual_pixel;
                  r=virtual_metacontent;
                  break;
                }
              p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
                x_modulo.remainder,y_modulo.remainder,1UL,1UL,virtual_nexus,
                exception);
              r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
              break;
            }
            case HorizontalTileVirtualPixelMethod:
            {
              if ((y_offset < 0) || (y_offset >= (ssize_t) cache_info->rows))
                {
                  p=virtual_pixel;
                  r=virtual_metacontent;
                  bre