// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 48/53



          /*
            Pixels are accessed directly from memory.
          */
          if (IsValidPixelOffset(y,cache_info->columns) == MagickFalse)
            return((Quantum *) NULL);
          offset=y*(MagickOffsetType) cache_info->columns+x;
          nexus_info->pixels=cache_info->pixels+(MagickOffsetType)
            cache_info->number_channels*offset;
          nexus_info->metacontent=(void *) NULL;
          if (cache_info->metacontent_extent != 0)
            nexus_info->metacontent=(unsigned char *) cache_info->metacontent+
              offset*(MagickOffsetType) cache_info->metacontent_extent;
          nexus_info->region.width=width;
          nexus_info->region.height=height;
          nexus_info->region.x=x;
          nexus_info->region.y=y;
          nexus_info->authentic_pixel_cache=MagickTrue;
          PrefetchPixelCacheNexusPixels(nexus_info,mode);
          return(nexus_info->pixels);
        }
    }
  /*
    Pixels are stored in a staging region until they are synced to the cache.
  */
  number_pixels=(MagickSizeType) width*height;
  length=MagickMax(number_pixels,MagickMax(cache_info->columns,
    cache_info->rows))*cache_info->number_channels*sizeof(*nexus_info->pixels);
  if (cache_info->metacontent_extent != 0)
    length+=number_pixels*cache_info->metacontent_extent;
  status=MagickTrue;
  if (nexus_info->cache == (Quantum *) NULL)
    status=AcquireCacheNexusPixels(cache_info,length,nexus_info,exception);
  else
    if (nexus_info->length < length)
      {
        RelinquishCacheNexusPixels(nexus_info);
        status=AcquireCacheNexusPixels(cache_info,length,nexus_info,exception);
      }
  if (status == MagickFalse)
    return((Quantum *) NULL);
  nexus_info->pixels=nexus_info->cache;
  nexus_info->metacontent=(void *) NULL;
  if (cache_info->metacontent_extent != 0)
    nexus_info->metacontent=(void *) (nexus_info->pixels+
      cache_info->number_channels*number_pixels);
  nexus_info->region.width=width;
  nexus_info->region.height=height;
  nexus_info->region.x=x;
  nexus_info->region.y=y;
  nexus_info->authentic_pixel_cache=cache_info->type == PingCache ?
    MagickTrue : MagickFalse;
  PrefetchPixelCacheNexusPixels(nexus_info,mode);
  return(nexus_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t P i x e l C a c h e V i r t u a l M e t h o d                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetPixelCacheVirtualMethod() sets the "virtual pixels" method for the
%  pixel cache and returns the previous setting.  A virtual pixel is any pixel
%  access that is outside the boundaries of the image cache.
%
%  The format of the SetPixelCacheVirtualMethod() method is:
%
%      VirtualPixelMethod SetPixelCacheVirtualMethod(Image *image,
%        const VirtualPixelMethod virtual_pixel_method,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o virtual_pixel_method: choose the type of virtual pixel.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickBooleanType SetCacheAlphaChannel(Image *image,const Quantum alpha,
  ExceptionInfo *exception)
{
  CacheView
    *magick_restrict image_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  image->alpha_trait=BlendPixelTrait;
  status=MagickTrue;
  image_view=AcquireVirtualCacheView(image,exception);  /* must be virtual */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      SetPixelAlpha(image,alpha,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    status=SyncCacheViewAuthenticPixels(image_view,exception);
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}