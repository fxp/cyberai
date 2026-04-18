// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 34/53



      /*
        Does the security policy require anonymous mapping for pixel cache?
      */
      cache_anonymous_memory=0;
      value=GetPolicyValue("pixel-cache-memory");
      if (value == (char *) NULL)
        value=GetPolicyValue("cache:memory-map");
      if (LocaleCompare(value,"anonymous") == 0)
        {
#if defined(MAGICKCORE_HAVE_MMAP) && defined(MAP_ANONYMOUS)
          cache_anonymous_memory=1;
#else
          (void) ThrowMagickException(exception,GetMagickModule(),
            MissingDelegateError,"DelegateLibrarySupportNotBuiltIn",
            "`%s' (policy requires anonymous memory mapping)",image->filename);
#endif
        }
      value=DestroyString(value);
    }
  if ((image->columns == 0) || (image->rows == 0))
    ThrowBinaryException(CacheError,"NoPixelsDefinedInCache",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (((MagickSizeType) image->columns > cache_info->width_limit) ||
      ((MagickSizeType) image->rows > cache_info->height_limit))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "WidthOrHeightExceedsLimit","`%s' (%.20gx%.20g) > (%.20gx%.20g)",
        image->filename, (double) image->columns, (double) image->rows,
        (double) cache_info->width_limit,(double) cache_info->height_limit);
      return(MagickFalse);
    }
  if (GetMagickResourceLimit(ListLengthResource) != MagickResourceInfinity)
    {
      length=GetImageListLength(image);
      if (AcquireMagickResource(ListLengthResource,length) == MagickFalse)
        ThrowBinaryException(ResourceLimitError,"ListLengthExceedsLimit",
          image->filename);
    }
  source_info=(*cache_info);
  source_info.file=(-1);
  (void) FormatLocaleString(cache_info->filename,MagickPathExtent,"%s[%.20g]",
    image->filename,(double) image->scene);
  cache_info->storage_class=image->storage_class;
  cache_info->colorspace=image->colorspace;
  cache_info->alpha_trait=image->alpha_trait;
  cache_info->channels=image->channels;
  cache_info->rows=image->rows;
  cache_info->columns=image->columns;
  status=ResetPixelChannelMap(image,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  cache_info->number_channels=GetPixelChannels(image);
  (void) memcpy(cache_info->channel_map,image->channel_map,MaxPixelChannels*
    sizeof(*image->channel_map));
  cache_info->metacontent_extent=image->metacontent_extent;
  cache_info->mode=mode;
  number_pixels=(MagickSizeType) cache_info->columns*cache_info->rows;
  packet_size=MagickMax(cache_info->number_channels,1)*sizeof(Quantum);
  if (image->metacontent_extent != 0)
    packet_size+=cache_info->metacontent_extent;
  if (CacheOverflowSanityCheckGetSize(number_pixels,packet_size,&length) != MagickFalse)
    {
      cache_info->storage_class=UndefinedClass;
      cache_info->length=0;
      ThrowBinaryException(ResourceLimitError,"PixelCacheAllocationFailed",
        image->filename);
    }
  columns=(size_t) (length/cache_info->rows/packet_size);
  if ((cache_info->columns != columns) || ((ssize_t) cache_info->columns < 0) ||
      ((ssize_t) cache_info->rows < 0))
    {
      cache_info->storage_class=UndefinedClass;
      cache_info->length=0;
      ThrowBinaryException(ResourceLimitError,"PixelCacheAllocationFailed",
        image->filename);
    }
  cache_info->length=length;
  if (image->ping != MagickFalse)
    {
      cache_info->type=PingCache;
      return(MagickTrue);
    }
  status=AcquireMagickResource(AreaResource,(MagickSizeType)
    cache_info->columns*cache_info->rows);
  if (cache_info->mode == PersistMode)
    status=MagickFalse;
  length=number_pixels*(cache_info->number_channels*sizeof(Quantum)+
    cache_info->metacontent_extent);
  if ((status != MagickFalse) &&
      (length == (MagickSizeType) ((size_t) length)) &&
      ((cache_info->type == UndefinedCache) ||
       (cache_info->type == MemoryCache)))
    {
      status=AcquireMagickResource(MemoryResource,cache_info->length);
      if (status != MagickFalse)
        {
          status=MagickTrue;
          if (cache_anonymous_memory <= 0)
            {
              cache_info->mapped=MagickFalse;
              cache_info->pixels=(Quantum *) MagickAssumeAligned(
                AcquireAlignedMemory(1,(size_t) cache_info->length));
            }
          else
            {
              cache_info->mapped=MagickTrue;
              cache_info->pixels=(Quantum *) MapBlob(-1,IOMode,0,(size_t)
                cache_info->length);
            }
          if (cache_info->pixels == (Quantum *) NULL)
            {
              cache_info->mapped=source_info.mapped;
              cache_info->pixels=source_info.pixels;
            }
          else
            {
              /*
                Create memory pixel cache.
              */
              cache_info->type=MemoryCache;
              cache_info->metacontent=(void *) NULL;
              if (cache_info->metacontent_extent != 0)
          