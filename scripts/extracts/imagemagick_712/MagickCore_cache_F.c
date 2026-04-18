// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 38/53

lename,const MagickBooleanType attach,MagickOffsetType *offset,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info,
    *magick_restrict clone_info;

  MagickBooleanType
    status;

  ssize_t
    page_size;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (void *) NULL);
  assert(filename != (const char *) NULL);
  assert(offset != (MagickOffsetType *) NULL);
  page_size=GetMagickPageSize();
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  CopyOpenCLBuffer(cache_info);
#endif
  if (attach != MagickFalse)
    {
      /*
        Attach existing persistent pixel cache.
      */
      if (cache_info->debug != MagickFalse)
        (void) LogMagickEvent(CacheEvent,GetMagickModule(),
          "attach persistent cache");
      (void) CopyMagickString(cache_info->cache_filename,filename,
        MagickPathExtent);
      cache_info->type=MapCache;
      cache_info->offset=(*offset);
      if (OpenPixelCache(image,ReadMode,exception) == MagickFalse)
        return(MagickFalse);
      *offset=(*offset+(MagickOffsetType) cache_info->length+page_size-
        ((MagickOffsetType) cache_info->length % page_size));
      return(MagickTrue);
    }
  /*
    Clone persistent pixel cache.
  */
  status=AcquireMagickResource(DiskResource,cache_info->length);
  if (status == MagickFalse)
    {
      cache_info->type=UndefinedCache;
      (void) memset(image->channel_map,0,MaxPixelChannels*
        sizeof(*image->channel_map));
      (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
        "CacheResourcesExhausted","`%s'",image->filename);
      return(MagickFalse);
    }
  clone_info=(CacheInfo *) ClonePixelCache(cache_info);
  clone_info->type=DiskCache;
  (void) CopyMagickString(clone_info->cache_filename,filename,MagickPathExtent);
  clone_info->file=(-1);
  clone_info->storage_class=cache_info->storage_class;
  clone_info->colorspace=cache_info->colorspace;
  clone_info->alpha_trait=cache_info->alpha_trait;
  clone_info->channels=cache_info->channels;
  clone_info->columns=cache_info->columns;
  clone_info->rows=cache_info->rows;
  clone_info->number_channels=cache_info->number_channels;
  clone_info->metacontent_extent=cache_info->metacontent_extent;
  clone_info->mode=PersistMode;
  clone_info->length=cache_info->length;
  (void) memcpy(clone_info->channel_map,cache_info->channel_map,
    MaxPixelChannels*sizeof(*cache_info->channel_map));
  clone_info->offset=(*offset);
  status=OpenPixelCacheOnDisk(clone_info,WriteMode);
  if (status != MagickFalse)
    status=ClonePixelCacheRepository(clone_info,cache_info,exception);
  *offset=(*offset+(MagickOffsetType) cache_info->length+page_size-
    ((MagickOffsetType) cache_info->length % page_size));
  clone_info=(CacheInfo *) DestroyPixelCache(clone_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   Q u e u e A u t h e n t i c P i x e l C a c h e N e x u s                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  QueueAuthenticPixelCacheNexus() allocates an region to store image pixels as
%  defined by the region rectangle and returns a pointer to the region.  This
%  region is subsequently transferred from the pixel cache with
%  SyncAuthenticPixelsCache().  A pointer to the pixels is returned if the
%  pixels are transferred, otherwise a NULL is returned.
%
%  The format of the QueueAuthenticPixelCacheNexus() method is:
%
%      Quantum *QueueAuthenticPixelCacheNexus(Image *image,const ssize_t x,
%        const ssize_t y,const size_t columns,const size_t rows,
%        const MagickBooleanType clone,NexusInfo *nexus_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o nexus_info: the cache nexus to set.
%
%    o clone: clone the pixel cache.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickPrivate Quantum *QueueAuthenticPixelCacheNexus(Image *image,
  const ssize_t x,const ssize_t y,const size_t columns,const size_t rows,
  const MagickBooleanType clone,NexusInfo *nexus_info,ExceptionInfo *exception)
{
