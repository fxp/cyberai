// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 47/53



static inline MagickBooleanType AcquireCacheNexusPixels(
  const CacheInfo *magick_restrict cache_info,const MagickSizeType length,
  NexusInfo *magick_restrict nexus_info,ExceptionInfo *exception)
{
  if (length != (MagickSizeType) ((size_t) length))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"PixelCacheAllocationFailed","`%s'",
        cache_info->filename);
      return(MagickFalse);
    }
  nexus_info->length=0;
  nexus_info->mapped=MagickFalse;
  if (cache_anonymous_memory <= 0)
    {
      nexus_info->cache=(Quantum *) MagickAssumeAligned(AcquireAlignedMemory(1,
        (size_t) length));
      if (nexus_info->cache != (Quantum *) NULL)
        (void) memset(nexus_info->cache,0,(size_t) length);
    }
  else
    {
      nexus_info->cache=(Quantum *) MapBlob(-1,IOMode,0,(size_t) length);
      if (nexus_info->cache != (Quantum *) NULL)
        nexus_info->mapped=MagickTrue;
    }
  if (nexus_info->cache == (Quantum *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"PixelCacheAllocationFailed","`%s'",
        cache_info->filename);
      return(MagickFalse);
    }
  nexus_info->length=length;
  return(MagickTrue);
}

static inline void PrefetchPixelCacheNexusPixels(const NexusInfo *nexus_info,
  const MapMode mode)
{
  if (nexus_info->length < CACHE_LINE_SIZE)
    return;
  if (mode == ReadMode)
    {
      MagickCachePrefetch((unsigned char *) nexus_info->pixels+CACHE_LINE_SIZE,
        0,1);
      return;
    }
  MagickCachePrefetch((unsigned char *) nexus_info->pixels+CACHE_LINE_SIZE,1,1);
}

static Quantum *SetPixelCacheNexusPixels(
  const CacheInfo *magick_restrict cache_info,const MapMode mode,
  const ssize_t x,const ssize_t y,const size_t width,const size_t height,
  const MagickBooleanType buffered,NexusInfo *magick_restrict nexus_info,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  MagickSizeType
    length,
    number_pixels;

  assert(cache_info != (const CacheInfo *) NULL);
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->type == UndefinedCache)
    return((Quantum *) NULL);
  assert(nexus_info->signature == MagickCoreSignature);
  (void) memset(&nexus_info->region,0,sizeof(nexus_info->region));
  if ((width == 0) || (height == 0))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
        "NoPixelsDefinedInCache","`%s'",cache_info->filename);
      return((Quantum *) NULL);
    }
  if (((MagickSizeType) width > cache_info->width_limit) ||
      ((MagickSizeType) height > cache_info->height_limit))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "WidthOrHeightExceedsLimit","`%s'",cache_info->filename);
      return((Quantum *) NULL);
    }
  if ((IsValidPixelOffset(x,width) == MagickFalse) ||
      (IsValidPixelOffset(y,height) == MagickFalse))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
        "InvalidPixel","`%s'",cache_info->filename);
      return((Quantum *) NULL);
    }
  if (((cache_info->type == MemoryCache) || (cache_info->type == MapCache)) &&
      (buffered == MagickFalse))
    {
      if (((x >= 0) && (y >= 0) &&
          (((ssize_t) height+y-1) < (ssize_t) cache_info->rows)) &&
          (((x == 0) && (width == cache_info->columns)) || ((height == 1) &&
          (((ssize_t) width+x-1) < (ssize_t) cache_info->columns))))
        {
          MagickOffsetType
            offset;