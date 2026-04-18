// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 49/53



MagickPrivate VirtualPixelMethod SetPixelCacheVirtualMethod(Image *image,
  const VirtualPixelMethod virtual_pixel_method,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  VirtualPixelMethod
    method;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  method=cache_info->virtual_pixel_method;
  cache_info->virtual_pixel_method=virtual_pixel_method;
  if ((image->columns != 0) && (image->rows != 0))
    switch (virtual_pixel_method)
    {
      case BackgroundVirtualPixelMethod:
      {
        if ((image->background_color.alpha_trait != UndefinedPixelTrait) &&
            ((image->alpha_trait & BlendPixelTrait) == 0))
          (void) SetCacheAlphaChannel(image,OpaqueAlpha,exception);
        if ((IsPixelInfoGray(&image->background_color) == MagickFalse) &&
            (IsGrayColorspace(image->colorspace) != MagickFalse))
          (void) SetImageColorspace(image,sRGBColorspace,exception);
        break;
      }
      case TransparentVirtualPixelMethod:
      {
        if ((image->alpha_trait & BlendPixelTrait) == 0)
          (void) SetCacheAlphaChannel(image,OpaqueAlpha,exception);
        break;
      }
      default:
        break;
    }
  return(method);
}

#if defined(MAGICKCORE_OPENCL_SUPPORT)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S y n c A u t h e n t i c O p e n C L B u f f e r                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncAuthenticOpenCLBuffer() makes sure that all the OpenCL operations have
%  been completed and updates the host memory.
%
%  The format of the SyncAuthenticOpenCLBuffer() method is:
%
%      void SyncAuthenticOpenCLBuffer(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/

static void CopyOpenCLBuffer(CacheInfo *magick_restrict cache_info)
{
  assert(cache_info != (CacheInfo *) NULL);
  assert(cache_info->signature == MagickCoreSignature);
  if ((cache_info->type != MemoryCache) ||
      (cache_info->opencl == (MagickCLCacheInfo) NULL))
    return;
  /*
    Ensure single threaded access to OpenCL environment.
  */
  LockSemaphoreInfo(cache_info->semaphore);
  cache_info->opencl=CopyMagickCLCacheInfo(cache_info->opencl);
  UnlockSemaphoreInfo(cache_info->semaphore);
}

MagickPrivate void SyncAuthenticOpenCLBuffer(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image != (const Image *) NULL);
  cache_info=(CacheInfo *) image->cache;
  CopyOpenCLBuffer(cache_info);
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S y n c A u t h e n t i c P i x e l C a c h e N e x u s                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncAuthenticPixelCacheNexus() saves the authentic image pixels to the
%  in-memory or disk cache.  The method returns MagickTrue if the pixel region
%  is synced, otherwise MagickFalse.
%
%  The format of the SyncAuthenticPixelCacheNexus() method is:
%
%      MagickBooleanType SyncAuthenticPixelCacheNexus(Image *image,
%        NexusInfo *nexus_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o nexus_info: the cache nexus to sync.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickPrivate MagickBooleanType SyncAuthenticPixelCacheNexus(Image *image,
  NexusInfo *magick_restrict nexus_info,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  MagickBooleanType
    status;