// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jxl.c
// Segment 2/11



#if defined(MAGICKCORE_JXL_DELEGATE)
/*
  Forward declarations.
*/
static MagickBooleanType
  WriteJXLImage(const ImageInfo *,Image *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s J X L                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsJXL() returns MagickTrue if the image format type, identified by the
%  magick string, is JXL.
%
%  The format of the IsJXL  method is:
%
%      MagickBooleanType IsJXL(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: compare image format pattern against these bytes.
%
%    o length: Specifies the length of the magick string.
%
*/
static MagickBooleanType IsJXL(const unsigned char *magick,const size_t length)
{
  JxlSignature
    signature = JxlSignatureCheck(magick,length);

  if ((signature == JXL_SIG_NOT_ENOUGH_BYTES) || (signature == JXL_SIG_INVALID))
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d J X L I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadJXLImage() reads a JXL image file and returns it.  It allocates
%  the memory necessary for the new Image structure and returns a pointer to
%  the new image.
%
%  The format of the ReadJXLImage method is:
%
%      Image *ReadJXLImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static void *JXLAcquireMemory(void *opaque,size_t size)
{
  unsigned char
    *data;

  data=(unsigned char *) AcquireQuantumMemory(size,sizeof(*data));
  if (data == (unsigned char *) NULL)
    {
      MemoryManagerInfo
        *memory_manager_info;

      memory_manager_info=(MemoryManagerInfo *) opaque;
      (void) ThrowMagickException(memory_manager_info->exception,
        GetMagickModule(),CoderError,"MemoryAllocationFailed","`%s'",
        memory_manager_info->image->filename);
    }
  return(data);
}

static inline StorageType JXLDataTypeToStorageType(Image *image,
  const JxlDataType data_type,ExceptionInfo *exception)
{
  switch (data_type)
  {
    case JXL_TYPE_FLOAT:
      return(FloatPixel);
    case JXL_TYPE_FLOAT16:
      (void) SetImageProperty(image,"quantum:format","floating-point",
        exception);
      return(FloatPixel);
    case JXL_TYPE_UINT16:
      return(ShortPixel);
    case JXL_TYPE_UINT8:
      return(CharPixel);
    default:
      return(UndefinedPixel);
  }
}

static inline OrientationType JXLOrientationToOrientation(
  const JxlOrientation orientation)
{
  switch (orientation)
  {
    default:
    case JXL_ORIENT_IDENTITY:
      return(TopLeftOrientation);
    case JXL_ORIENT_FLIP_HORIZONTAL:
      return(TopRightOrientation);
    case JXL_ORIENT_ROTATE_180:
      return(BottomRightOrientation);
    case JXL_ORIENT_FLIP_VERTICAL:
      return(BottomLeftOrientation);
    case JXL_ORIENT_TRANSPOSE:
      return(LeftTopOrientation);
    case JXL_ORIENT_ROTATE_90_CW:
      return(RightTopOrientation);
    case JXL_ORIENT_ANTI_TRANSPOSE:
      return(RightBottomOrientation);
    case JXL_ORIENT_ROTATE_90_CCW:
      return(LeftBottomOrientation);
  }
}

static void JXLRelinquishMemory(void *magick_unused(opaque),void *address)
{
  magick_unreferenced(opaque);
  (void) RelinquishMagickMemory(address);
}

static inline void JXLSetMemoryManager(JxlMemoryManager *memory_manager,
  MemoryManagerInfo *memory_manager_info,Image *image,ExceptionInfo *exception)
{
  memory_manager_info->image=image;
  memory_manager_info->exception=exception;
  memory_manager->opaque=memory_manager_info;
  memory_manager->alloc=JXLAcquireMemory;
  memory_manager->free=JXLRelinquishMemory;
}