// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 26/53



  MagickSizeType
    length,
    number_pixels;

  NexusInfo
    *magick_restrict virtual_nexus;

  Quantum
    *magick_restrict pixels,
    *magick_restrict q,
    virtual_pixel[MaxPixelChannels];

  ssize_t
    i,
    u,
    v;

  unsigned char
    *magick_restrict s;

  void
    *magick_restrict virtual_metacontent;

  /*
    Acquire pixels.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->type == UndefinedCache)
    return((const Quantum *) NULL);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  CopyOpenCLBuffer(cache_info);
#endif
  pixels=SetPixelCacheNexusPixels(cache_info,ReadMode,x,y,columns,rows,
    ((image->channels & WriteMaskChannel) != 0) ||
    ((image->channels & CompositeMaskChannel) != 0) ? MagickTrue : MagickFalse,
    nexus_info,exception);
  if (pixels == (Quantum *) NULL)
    return((const Quantum *) NULL);
  if (IsValidPixelOffset(nexus_info->region.y,cache_info->columns) == MagickFalse)
    return((const Quantum *) NULL);
  offset=nexus_info->region.y*(MagickOffsetType) cache_info->columns;
  if (IsOffsetOverflow(offset,(MagickOffsetType) nexus_info->region.x) == MagickFalse)
    return((const Quantum *) NULL);
  offset+=nexus_info->region.x;
  length=(MagickSizeType) (nexus_info->region.height-1L)*cache_info->columns+
    nexus_info->region.width-1L;
  number_pixels=(MagickSizeType) cache_info->columns*cache_info->rows;
  if ((offset >= 0) && (((MagickSizeType) offset+length) < number_pixels))
    if ((x >= 0) && ((x+(ssize_t) columns-1) < (ssize_t) cache_info->columns) &&
        (y >= 0) && ((y+(ssize_t) rows-1) < (ssize_t) cache_info->rows))
      {
        MagickBooleanType
          status;