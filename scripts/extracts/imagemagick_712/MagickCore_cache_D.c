// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 36/53



      /*
        Distribute the pixel cache to a remote server.
      */
      server_info=AcquireDistributeCacheInfo(exception);
      if (server_info != (DistributeCacheInfo *) NULL)
        {
          status=OpenDistributePixelCache(server_info,image);
          if (status == MagickFalse)
            {
              ThrowFileException(exception,CacheError,"UnableToOpenPixelCache",
                GetDistributeCacheHostname(server_info));
              server_info=DestroyDistributeCacheInfo(server_info);
            }
          else
            {
              /*
                Create a distributed pixel cache.
              */
              status=MagickTrue;
              cache_info->type=DistributedCache;
              cache_info->server_info=server_info;
              (void) FormatLocaleString(cache_info->cache_filename,
                MagickPathExtent,"%s:%d",GetDistributeCacheHostname(
                (DistributeCacheInfo *) cache_info->server_info),
                GetDistributeCachePort((DistributeCacheInfo *)
                cache_info->server_info));
              if ((source_info.storage_class != UndefinedClass) &&
                  (mode != ReadMode))
                {
                  status=ClonePixelCacheRepository(cache_info,&source_info,
                    exception);
                  RelinquishPixelCachePixels(&source_info);
                }
              if (cache_info->debug != MagickFalse)
                {
                  (void) FormatMagickSize(cache_info->length,MagickFalse,"B",
                    MagickPathExtent,format);
                  type=CommandOptionToMnemonic(MagickCacheOptions,(ssize_t)
                    cache_info->type);
                  (void) FormatLocaleString(message,MagickPathExtent,
                    "open %s (%s[%d], %s, %.20gx%.20gx%.20g %s)",
                    cache_info->filename,cache_info->cache_filename,
                    GetDistributeCacheFile((DistributeCacheInfo *)
                    cache_info->server_info),type,(double) cache_info->columns,
                    (double) cache_info->rows,(double)
                    cache_info->number_channels,format);
                  (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",
                    message);
                }
              if (status == 0)
                {
                  if ((source_info.storage_class != UndefinedClass) &&
                      (mode != ReadMode))
                    RelinquishPixelCachePixels(&source_info);
                  cache_info->type=UndefinedCache;
                  return(MagickFalse);
                }
              return(MagickTrue);
            }
        }
      if ((source_info.storage_class != UndefinedClass) && (mode != ReadMode))
        RelinquishPixelCachePixels(&source_info);
      cache_info->type=UndefinedCache;
      (void) memset(image->channel_map,0,MaxPixelChannels*
        sizeof(*image->channel_map));
      (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
        "CacheResourcesExhausted","`%s'",image->filename);
      return(MagickFalse);
    }
  /*
    Create pixel cache on disk.
  */
  if (status == MagickFalse)
    {
      if ((source_info.storage_class != UndefinedClass) && (mode != ReadMode))
        RelinquishPixelCachePixels(&source_info);
      cache_info->type=UndefinedCache;
      (void) memset(image->channel_map,0,MaxPixelChannels*
        sizeof(*image->channel_map));
      (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
        "CacheResourcesExhausted","`%s'",image->filename);
      return(MagickFalse);
    }
  if ((source_info.storage_class != UndefinedClass) && (mode != ReadMode) &&
      (cache_info->mode != PersistMode))
    {
      (void) ClosePixelCacheOnDisk(cache_info);
      *cache_info->cache_filename='\0';
    }
  if (OpenPixelCacheOnDisk(cache_info,mode) == MagickFalse)
    {
      if ((source_info.storage_class != UndefinedClass) && (mode != ReadMode))
        RelinquishPixelCachePixels(&source_info);
      cache_info->type=UndefinedCache;
      ThrowFileException(exception,CacheError,"UnableToOpenPixelCache",
        image->filename);
      return(MagickFalse);
    }
  status=SetPixelCacheExtent(image,(MagickSizeType) cache_info->offset+
    cache_info->length);
  if (status == MagickFalse)
    {
      if ((source_info.storage_class != UndefinedClass) && (mode != ReadMode))
        RelinquishPixelCachePixels(&source_info);
      cache_info->type=UndefinedCache;
      ThrowFileException(exception,CacheError,"UnableToExtendCache",
        image->filename);
      return(MagickFalse);
    }
  cache_info->type=DiskCache;
  length=number_pixels*(cache_info->number_channels*sizeof(Quantum)+
    cache_info->metacontent_extent);
  if (length == (MagickSizeType) ((size_t) length))
    {
      status=AcquireMagickResource(MapResource,cache_info->length);
      if (status != MagickFalse)
        {
          cache_info->pixels=(Quantum *) MapBlob(cache_info->file,