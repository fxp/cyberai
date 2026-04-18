// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 41/53



  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->methods.queue_authentic_pixels_handler !=
      (QueueAuthenticPixelsHandler) NULL)
    {
      pixels=cache_info->methods.queue_authentic_pixels_handler(image,x,y,
        columns,rows,exception);
      return(pixels);
    }
  assert(id < (int) cache_info->number_threads);
  pixels=QueueAuthenticPixelCacheNexus(image,x,y,columns,rows,MagickFalse,
    cache_info->nexus_info[id],exception);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e a d P i x e l C a c h e M e t a c o n t e n t                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPixelCacheMetacontent() reads metacontent from the specified region of
%  the pixel cache.
%
%  The format of the ReadPixelCacheMetacontent() method is:
%
%      MagickBooleanType ReadPixelCacheMetacontent(CacheInfo *cache_info,
%        NexusInfo *nexus_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o cache_info: the pixel cache.
%
%    o nexus_info: the cache nexus to read the metacontent.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickOffsetType ReadPixelCacheRegion(
  const CacheInfo *magick_restrict cache_info,const MagickOffsetType offset,
  const MagickSizeType length,unsigned char *magick_restrict buffer)
{
  MagickOffsetType
    i;

  ssize_t
    count = 0;

#if !defined(MAGICKCORE_HAVE_PREAD)
  if (lseek(cache_info->file,offset,SEEK_SET) < 0)
    return((MagickOffsetType) -1);
#endif
  for (i=0; i < (MagickOffsetType) length; i+=count)
  {
#if !defined(MAGICKCORE_HAVE_PREAD)
    count=read(cache_info->file,buffer+i,(size_t) MagickMin(length-
      (MagickSizeType) i,(size_t) MagickMaxBufferExtent));
#else
    count=pread(cache_info->file,buffer+i,(size_t) MagickMin(length-
      (MagickSizeType) i,(size_t) MagickMaxBufferExtent),offset+i);
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

static MagickBooleanType ReadPixelCacheMetacontent(
  CacheInfo *magick_restrict cache_info,NexusInfo *magick_restrict nexus_info,
  ExceptionInfo *exception)
{
  MagickOffsetType
    count,
    offset;

  MagickSizeType
    extent,
    length;

  ssize_t
    y;

  unsigned char
    *magick_restrict q;

  size_t
    rows;

  if (cache_info->metacontent_extent == 0)
    return(MagickFalse);
  if (nexus_info->authentic_pixel_cache != MagickFalse)
    return(MagickTrue);
  if (IsValidPixelOffset(nexus_info->region.y,cache_info->columns) == MagickFalse)
    return(MagickFalse);
  offset=nexus_info->region.y*(MagickOffsetType) cache_info->columns+
    nexus_info->region.x;
  length=(MagickSizeType) nexus_info->region.width*
    cache_info->metacontent_extent;
  extent=length*nexus_info->region.height;
  rows=nexus_info->region.height;
  y=0;
  q=(unsigned char *) nexus_info->metacontent;
  switch (cache_info->type)
  {
    case MemoryCache:
    case MapCache:
    {
      unsigned char
        *magick_restrict p;