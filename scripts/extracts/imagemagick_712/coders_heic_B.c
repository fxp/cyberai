// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 2/17



/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/property.h"
#include "MagickCore/display.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/transform.h"
#include "MagickCore/distort.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile-private.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/static.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/module.h"
#include "MagickCore/utility.h"
#define HEIC_COMPUTE_NUMERIC_VERSION(major,minor,patch) \
  (((major) << 24) | ((minor) << 16) | ((patch) << 8) | 0)
#if defined(MAGICKCORE_HEIC_DELEGATE)
#include <libheif/heif.h>
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,17,0)
#include <libheif/heif_properties.h>
#endif
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,20,0)
#include <libheif/heif_sequences.h>
#endif
#endif

#if defined(MAGICKCORE_HEIC_DELEGATE)
/*
  Forward declarations.
*/
static MagickBooleanType
  WriteHEICImage(const ImageInfo *,Image *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d H E I C I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadHEICImage retrieves an image via a file descriptor, decodes the image,
%  and returns it.  It allocates the memory necessary for the new Image
%  structure and returns a pointer to the new image.
%
%  The format of the ReadHEICImage method is:
%
%      Image *ReadHEICImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,19,0)
static inline void HEICSetUint32SecurityLimit(const ImageInfo *image_info,
  const char *name,uint32_t *value)
{
  const char
    *option;

  option=GetImageOption(image_info,name);
  if (option == (const char*) NULL)
    return;
  *value=strtoul(option,(char **) NULL,10);
}

static inline void HEICSetUint64SecurityLimit(const ImageInfo *image_info,
  const char *name,uint64_t *value)
{
  const char
    *option;

  option=GetImageOption(image_info,name);
  if (option == (const char*) NULL)
    return;
  *value=strtoull(option,(char **) NULL,10);
}

static inline void HEICSecurityLimits(const ImageInfo *image_info,
  struct heif_context *heif_context)
{
  int
    height_limit,
    max_profile_size,
    width_limit;

  struct heif_security_limits
    *security_limits;