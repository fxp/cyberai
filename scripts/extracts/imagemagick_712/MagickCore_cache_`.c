// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 32/53



  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->storage_class == UndefinedClass)
    return((Quantum *) NULL);
  return((const Quantum *) nexus_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   M a s k P i x e l C a c h e N e x u s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MaskPixelCacheNexus() masks the cache nexus as defined by the composite mask.
%  The method returns MagickTrue if the pixel region is masked, otherwise
%  MagickFalse.
%
%  The format of the MaskPixelCacheNexus() method is:
%
%      MagickBooleanType MaskPixelCacheNexus(Image *image,
%        NexusInfo *nexus_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o nexus_info: the cache nexus to clip.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline Quantum ApplyPixelCompositeMask(const Quantum p,
  const MagickRealType alpha,const Quantum q,const MagickRealType beta)
{
  double
    gamma;

  if (fabs((double) (alpha-(double) TransparentAlpha)) < MagickEpsilon)
    return(q);
  gamma=1.0-QuantumScale*QuantumScale*alpha*beta;
  gamma=MagickSafeReciprocal(gamma);
  return(ClampToQuantum(gamma*MagickOver_((double) p,alpha,(double) q,beta)));
}

static MagickBooleanType MaskPixelCacheNexus(Image *image,NexusInfo *nexus_info,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  Quantum
    *magick_restrict p,
    *magick_restrict q;

  ssize_t
    y;

  /*
    Apply composite mask.
  */
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((image->channels & CompositeMaskChannel) == 0)
    return(MagickTrue);
  if ((nexus_info->region.width == 0) || (nexus_info->region.height == 0))
    return(MagickTrue);
  cache_info=(CacheInfo *) image->cache;
  if (cache_info == (Cache) NULL)
    return(MagickFalse);
  p=GetAuthenticPixelCacheNexus(image,nexus_info->region.x,nexus_info->region.y,
    nexus_info->region.width,nexus_info->region.height,
    nexus_info->virtual_nexus,exception);
  q=nexus_info->pixels;
  if ((p == (Quantum *) NULL) || (q == (Quantum *) NULL))
    return(MagickFalse);
  for (y=0; y < (ssize_t) nexus_info->region.height; y++)
  {
    ssize_t
      x;

    for (x=0; x < (ssize_t) nexus_info->region.width; x++)
    {
      double
        alpha;

      ssize_t
        i;

      alpha=(double) GetPixelCompositeMask(image,p);
      for (i=0; i < (ssize_t) image->number_channels; i++)
      {
        PixelChannel channel = GetPixelChannelChannel(image,i);
        PixelTrait traits = GetPixelChannelTraits(image,channel);
        if ((traits & UpdatePixelTrait) == 0)
          continue;
        q[i]=ApplyPixelCompositeMask(q[i],alpha,p[i],GetPixelAlpha(image,p));
      }
      p+=(ptrdiff_t) GetPixelChannels(image);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   O p e n P i x e l C a c h e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpenPixelCache() allocates the pixel cache.  This includes defining the cache
%  dimensions, allocating space for the image pixels and optionally the
%  metacontent, and memory mapping the cache if it is disk based.  The cache
%  nexus array is initialized as well.
%
%  The format of the OpenPixelCache() method is:
%
%      MagickBooleanType OpenPixelCache(Image *image,const MapMode mode,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o mode: ReadMode, WriteMode, or IOMode.
%
%    o exception: return any errors or warnings in this structure.
%
*/