// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 33/53



static inline MagickBooleanType CacheOverflowSanityCheckGetSize(
  const MagickSizeType count,const size_t quantum,MagickSizeType *const extent)
{
  MagickSizeType
    length;

  if ((count == 0) || (quantum == 0))
    return(MagickTrue);
  length=count*quantum;
  if (quantum != (length/count))
    {
      errno=ENOMEM;
      return(MagickTrue);
    }
  if (extent != NULL)
    *extent=length;
  return(MagickFalse);
}

static MagickBooleanType OpenPixelCacheOnDisk(CacheInfo *cache_info,
  const MapMode mode)
{
  int
    file;

  /*
    Open pixel cache on disk.
  */
  if ((cache_info->file != -1) && (cache_info->disk_mode == mode))
    return(MagickTrue);  /* cache already open and in the proper mode */
  if (*cache_info->cache_filename == '\0')
    file=AcquireUniqueFileResource(cache_info->cache_filename);
  else
    switch (mode)
    {
      case ReadMode:
      {
        file=open_utf8(cache_info->cache_filename,O_RDONLY | O_BINARY,0);
        break;
      }
      case WriteMode:
      {
        file=open_utf8(cache_info->cache_filename,O_WRONLY | O_CREAT |
          O_BINARY | O_EXCL,S_MODE);
        if (file == -1)
          file=open_utf8(cache_info->cache_filename,O_WRONLY | O_BINARY,S_MODE);
        break;
      }
      case IOMode:
      default:
      {
        file=open_utf8(cache_info->cache_filename,O_RDWR | O_CREAT | O_BINARY |
          O_EXCL,S_MODE);
        if (file == -1)
          file=open_utf8(cache_info->cache_filename,O_RDWR | O_BINARY,S_MODE);
        break;
      }
    }
  if (file == -1)
    return(MagickFalse);
  (void) AcquireMagickResource(FileResource,1);
  if (cache_info->file != -1)
    (void) ClosePixelCacheOnDisk(cache_info);
  cache_info->file=file;
  cache_info->disk_mode=mode;
  return(MagickTrue);
}

static inline MagickOffsetType WritePixelCacheRegion(
  const CacheInfo *magick_restrict cache_info,const MagickOffsetType offset,
  const MagickSizeType length,const unsigned char *magick_restrict buffer)
{
  MagickOffsetType
    i;

  ssize_t
    count = 0;

#if !defined(MAGICKCORE_HAVE_PWRITE)
  if (lseek(cache_info->file,offset,SEEK_SET) < 0)
    return((MagickOffsetType) -1);
#endif
  for (i=0; i < (MagickOffsetType) length; i+=count)
  {
#if !defined(MAGICKCORE_HAVE_PWRITE)
    count=write(cache_info->file,buffer+i,(size_t) MagickMin(length-
      (MagickSizeType) i,MagickMaxBufferExtent));
#else
    count=pwrite(cache_info->file,buffer+i,(size_t) MagickMin(length-
      (MagickSizeType) i,MagickMaxBufferExtent),offset+i);
#endif
    if (count <= 0)
      {
        count=0;
        if (errno != EINTR)
          break;
      }
  }
  return(i);
}

static MagickBooleanType SetPixelCacheExtent(Image *image,MagickSizeType length)
{
  CacheInfo
    *magick_restrict cache_info;

  MagickOffsetType
    offset;

  cache_info=(CacheInfo *) image->cache;
  if (cache_info->debug != MagickFalse)
    {
      char
        format[MagickPathExtent],
        message[MagickPathExtent];

      (void) FormatMagickSize(length,MagickFalse,"B",MagickPathExtent,format);
      (void) FormatLocaleString(message,MagickPathExtent,
        "extend %s (%s[%d], disk, %s)",cache_info->filename,
        cache_info->cache_filename,cache_info->file,format);
      (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",message);
    }
  if (length != (MagickSizeType) ((MagickOffsetType) length))
    return(MagickFalse);
  offset=(MagickOffsetType) lseek(cache_info->file,0,SEEK_END);
  if (offset < 0)
    return(MagickFalse);
  if ((MagickSizeType) offset < length)
    {
      MagickOffsetType
        count,
        extent;

      extent=(MagickOffsetType) length-1;
      count=WritePixelCacheRegion(cache_info,extent,1,(const unsigned char *)
        "");
      if (count != 1)
        return(MagickFalse);
#if defined(MAGICKCORE_HAVE_POSIX_FALLOCATE)
      if (cache_info->synchronize != MagickFalse)
        if (posix_fallocate(cache_info->file,offset+1,extent-offset) != 0)
          return(MagickFalse);
#endif
    }
  offset=(MagickOffsetType) lseek(cache_info->file,0,SEEK_SET);
  if (offset < 0)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType OpenPixelCache(Image *image,const MapMode mode,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info,
    source_info;

  char
    format[MagickPathExtent],
    message[MagickPathExtent];

  const char
    *hosts,
    *type;

  MagickBooleanType
    status;

  MagickSizeType
    length = 0,
    number_pixels;

  size_t
    columns,
    packet_size;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (cache_anonymous_memory < 0)
    {
      char
        *value;