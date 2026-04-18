// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 29/53

ak;
                }
              x_modulo=VirtualPixelModulo(x_offset,cache_info->columns);
              y_modulo=VirtualPixelModulo(y_offset,cache_info->rows);
              p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
                x_modulo.remainder,y_modulo.remainder,1UL,1UL,virtual_nexus,
                exception);
              r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
              break;
            }
            case VerticalTileVirtualPixelMethod:
            {
              if ((x_offset < 0) || (x_offset >= (ssize_t) cache_info->columns))
                {
                  p=virtual_pixel;
                  r=virtual_metacontent;
                  break;
                }
              x_modulo=VirtualPixelModulo(x_offset,cache_info->columns);
              y_modulo=VirtualPixelModulo(y_offset,cache_info->rows);
              p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
                x_modulo.remainder,y_modulo.remainder,1UL,1UL,virtual_nexus,
                exception);
              r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
              break;
            }
          }
          if (p == (const Quantum *) NULL)
            break;
          (void) memcpy(q,p,(size_t) (cache_info->number_channels*length*
            sizeof(*p)));
          q+=(ptrdiff_t) cache_info->number_channels;
          if ((s != (void *) NULL) && (r != (const void *) NULL))
            {
              (void) memcpy(s,r,(size_t) cache_info->metacontent_extent);
              s+=(ptrdiff_t) cache_info->metacontent_extent;
            }
          continue;
        }
      /*
        Transfer a run of pixels.
      */
      p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,x_offset,y_offset,
        (size_t) length,1UL,virtual_nexus,exception);
      if (p == (const Quantum *) NULL)
        break;
      r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
      (void) memcpy(q,p,(size_t) (cache_info->number_channels*length*
        sizeof(*p)));
      q+=(ptrdiff_t) cache_info->number_channels*length;
      if ((r != (void *) NULL) && (s != (const void *) NULL))
        {
          (void) memcpy(s,r,(size_t) length);
          s+=(ptrdiff_t) length*cache_info->metacontent_extent;
        }
    }
    if (u < (ssize_t) columns)
      break;
  }
  /*
    Free resources.
  */
  if (virtual_metacontent != (void *) NULL)
    virtual_metacontent=(void *) RelinquishMagickMemory(virtual_metacontent);
  if (v < (ssize_t) rows)
    return((const Quantum *) NULL);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t V i r t u a l P i x e l C a c h e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualPixelCache() get virtual pixels from the in-memory or disk pixel
%  cache as defined by the geometry parameters.   A pointer to the pixels
%  is returned if the pixels are transferred, otherwise a NULL is returned.
%
%  The format of the GetVirtualPixelCache() method is:
%
%      const Quantum *GetVirtualPixelCache(const Image *image,
%        const VirtualPixelMethod virtual_pixel_method,const ssize_t x,
%        const ssize_t y,const size_t columns,const size_t rows,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o virtual_pixel_method: the virtual pixel method.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static const Quantum *GetVirtualPixelCache(const Image *image,
  const VirtualPixelMethod virtual_pixel_method,const ssize_t x,const ssize_t y,
  const size_t columns,const size_t rows,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  const Quantum
    *magick_restrict p;