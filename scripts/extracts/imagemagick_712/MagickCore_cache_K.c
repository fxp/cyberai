// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 43/53



  Quantum
    *magick_restrict q;

  size_t
    number_channels,
    rows;

  ssize_t
    y;

  if (nexus_info->authentic_pixel_cache != MagickFalse)
    return(MagickTrue);
  if (IsValidPixelOffset(nexus_info->region.y,cache_info->columns) == MagickFalse)
    return(MagickFalse);
  offset=nexus_info->region.y*(MagickOffsetType) cache_info->columns;
  if ((ssize_t) (offset/cache_info->columns) != nexus_info->region.y)
    return(MagickFalse);
  offset+=nexus_info->region.x;
  number_channels=cache_info->number_channels;
  length=(MagickSizeType) number_channels*nexus_info->region.width*
    sizeof(Quantum);
  if ((length/number_channels/sizeof(Quantum)) != nexus_info->region.width)
    return(MagickFalse);
  rows=nexus_info->region.height;
  extent=length*rows;
  if ((extent == 0) || ((extent/length) != rows))
    return(MagickFalse);
  y=0;
  q=nexus_info->pixels;
  switch (cache_info->type)
  {
    case MemoryCache:
    case MapCache:
    {
      Quantum
        *magick_restrict p;

      /*
        Read pixels from memory.
      */
      if ((cache_info->columns == nexus_info->region.width) &&
          (extent == (MagickSizeType) ((size_t) extent)))
        {
          length=extent;
          rows=1UL;
        }
      p=cache_info->pixels+(MagickOffsetType) cache_info->number_channels*
        offset;
      for (y=0; y < (ssize_t) rows; y++)
      {
        (void) memcpy(q,p,(size_t) length);
        p+=(ptrdiff_t) cache_info->number_channels*cache_info->columns;
        q+=(ptrdiff_t) cache_info->number_channels*nexus_info->region.width;
      }
      break;
    }
    case DiskCache:
    {
      /*
        Read pixels from disk.
      */
      LockSemaphoreInfo(cache_info->file_semaphore);
      if (OpenPixelCacheOnDisk(cache_info,IOMode) == MagickFalse)
        {
          ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
            cache_info->cache_filename);
          UnlockSemaphoreInfo(cache_info->file_semaphore);
          return(MagickFalse);
        }
      if ((cache_info->columns == nexus_info->region.width) &&
          (extent <= MagickMaxBufferExtent))
        {
          length=extent;
          rows=1UL;
        }
      for (y=0; y < (ssize_t) rows; y++)
      {
        count=ReadPixelCacheRegion(cache_info,cache_info->offset+offset*
          (MagickOffsetType) cache_info->number_channels*(MagickOffsetType)
          sizeof(*q),length,(unsigned char *) q);
        if (count != (MagickOffsetType) length)
          break;
        offset+=(MagickOffsetType) cache_info->columns;
        q+=(ptrdiff_t) cache_info->number_channels*nexus_info->region.width;
      }
      if (IsFileDescriptorLimitExceeded() != MagickFalse)
        (void) ClosePixelCacheOnDisk(cache_info);
      UnlockSemaphoreInfo(cache_info->file_semaphore);
      break;
    }
    case DistributedCache:
    {
      RectangleInfo
        region;