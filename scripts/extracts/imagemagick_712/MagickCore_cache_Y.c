// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/cache.c
// Segment 25/53



  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->methods.get_virtual_metacontent_from_handler != (GetVirtualMetacontentFromHandler) NULL)
    {
      metacontent=cache_info->methods.get_virtual_metacontent_from_handler(
        image);
      if (metacontent != (const void *) NULL)
        return(metacontent);
    }
  assert(id < (int) cache_info->number_threads);
  metacontent=GetVirtualMetacontentFromNexus(cache_info,
    cache_info->nexus_info[id]);
  return(metacontent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t V i r t u a l P i x e l C a c h e N e x u s                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualPixelCacheNexus() gets virtual pixels from the in-memory or disk
%  pixel cache as defined by the geometry parameters.   A pointer to the pixels
%  is returned if the pixels are transferred, otherwise a NULL is returned.
%
%  The format of the GetVirtualPixelCacheNexus() method is:
%
%      Quantum *GetVirtualPixelCacheNexus(const Image *image,
%        const VirtualPixelMethod method,const ssize_t x,const ssize_t y,
%        const size_t columns,const size_t rows,NexusInfo *nexus_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o virtual_pixel_method: the virtual pixel method.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o nexus_info: the cache nexus to acquire.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static ssize_t
  DitherMatrix[64] =
  {
     0,  48,  12,  60,   3,  51,  15,  63,
    32,  16,  44,  28,  35,  19,  47,  31,
     8,  56,   4,  52,  11,  59,   7,  55,
    40,  24,  36,  20,  43,  27,  39,  23,
     2,  50,  14,  62,   1,  49,  13,  61,
    34,  18,  46,  30,  33,  17,  45,  29,
    10,  58,   6,  54,   9,  57,   5,  53,
    42,  26,  38,  22,  41,  25,  37,  21
  };

static inline ssize_t DitherX(const ssize_t x,const size_t columns)
{
  ssize_t
    index;

  index=x+DitherMatrix[x & 0x07]-32L;
  if (index < 0L)
    return(0L);
  if (index >= (ssize_t) columns)
    return((ssize_t) columns-1L);
  return(index);
}

static inline ssize_t DitherY(const ssize_t y,const size_t rows)
{
  ssize_t
    index;

  index=y+DitherMatrix[y & 0x07]-32L;
  if (index < 0L)
    return(0L);
  if (index >= (ssize_t) rows)
    return((ssize_t) rows-1L);
  return(index);
}

static inline ssize_t EdgeX(const ssize_t x,const size_t columns)
{
  if (x < 0L)
    return(0L);
  if (x >= (ssize_t) columns)
    return((ssize_t) (columns-1));
  return(x);
}

static inline ssize_t EdgeY(const ssize_t y,const size_t rows)
{
  if (y < 0L)
    return(0L);
  if (y >= (ssize_t) rows)
    return((ssize_t) (rows-1));
  return(y);
}

static inline MagickBooleanType IsOffsetOverflow(const MagickOffsetType x,
  const MagickOffsetType y)
{
  if (((y > 0) && (x > ((MagickOffsetType) MAGICK_SSIZE_MAX-y))) ||
      ((y < 0) && (x < ((MagickOffsetType) MAGICK_SSIZE_MIN-y))))
    return(MagickFalse);
  return(MagickTrue);
}

static inline ssize_t RandomX(RandomInfo *random_info,const size_t columns)
{
  return((ssize_t) (columns*GetPseudoRandomValue(random_info)));
}

static inline ssize_t RandomY(RandomInfo *random_info,const size_t rows)
{
  return((ssize_t) (rows*GetPseudoRandomValue(random_info)));
}

static inline MagickModulo VirtualPixelModulo(const ssize_t offset,
  const size_t extent)
{
  MagickModulo
    modulo;

  modulo.quotient=offset;
  modulo.remainder=0;
  if (extent != 0)
    {
      modulo.quotient=offset/((ssize_t) extent);
      modulo.remainder=offset % ((ssize_t) extent);
    }
  if ((modulo.remainder != 0) && ((offset ^ ((ssize_t) extent)) < 0))
    {
      modulo.quotient-=1;
      modulo.remainder+=((ssize_t) extent);
    }
  return(modulo);
}

MagickPrivate const Quantum *GetVirtualPixelCacheNexus(const Image *image,
  const VirtualPixelMethod virtual_pixel_method,const ssize_t x,const ssize_t y,
  const size_t columns,const size_t rows,NexusInfo *nexus_info,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const Quantum
    *magick_restrict p;

  const void
    *magick_restrict r;

  MagickOffsetType
    offset;