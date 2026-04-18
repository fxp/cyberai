// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 35/53

      cache_info->metacontent=(void *) (cache_info->pixels+
                  cache_info->number_channels*number_pixels);
              if ((source_info.storage_class != UndefinedClass) &&
                  (mode != ReadMode))
                {
                  status=ClonePixelCacheRepository(cache_info,&source_info,
                    exception);
                  RelinquishPixelCachePixels(&source_info);
                }
              if (cache_info->debug != MagickFalse)
                {
                  (void) FormatMagickSize(cache_info->length,MagickTrue,"B",
                    MagickPathExtent,format);
                  type=CommandOptionToMnemonic(MagickCacheOptions,(ssize_t)
                    cache_info->type);
                  (void) FormatLocaleString(message,MagickPathExtent,
                    "open %s (%s %s, %.20gx%.20gx%.20g %s)",
                    cache_info->filename,cache_info->mapped != MagickFalse ?
                    "Anonymous" : "Heap",type,(double) cache_info->columns,
                    (double) cache_info->rows,(double)
                    cache_info->number_channels,format);
                  (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",
                    message);
                }
              cache_info->storage_class=image->storage_class;
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
    }
  status=AcquireMagickResource(DiskResource,cache_info->length);
  hosts=(const char *) GetImageRegistry(StringRegistryType,"cache:hosts",
    exception);
  if ((status == MagickFalse) && (hosts != (const char *) NULL))
    {
      DistributeCacheInfo
        *server_info;