// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 7/18

ngth of the magick string.
%
*/
static MagickBooleanType IsGIF(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (LocaleNCompare((char *) magick,"GIF8",4) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b B l o c k                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobBlock() reads data from the image file and returns it.  The
%  amount of data is determined by first reading a count byte.  The number
%  of bytes read is returned.
%
%  The format of the ReadBlobBlock method is:
%
%      ssize_t ReadBlobBlock(Image *image,unsigned char *data)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o data:  Specifies an area to place the information requested from
%      the file.
%
*/
static ssize_t ReadBlobBlock(Image *image,unsigned char *data)
{
  ssize_t
    count;

  unsigned char
    block_count = 0;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(data != (unsigned char *) NULL);
  count=ReadBlob(image,1,&block_count);
  if (count != 1)
    return(0);
  count=ReadBlob(image,(size_t) block_count,data);
  if (count != (ssize_t) block_count)
    return(0);
  return(count);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d G I F I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadGIFImage() reads a Compuserve Graphics image file and returns it.
%  It allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadGIFImage method is:
%
%      Image *ReadGIFImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static void *DestroyGIFProfile(void *profile)
{
  return((void *) DestroyStringInfo((StringInfo *) profile));
}

static MagickBooleanType PingGIFImage(Image *image,ExceptionInfo *exception)
{
  unsigned char
    buffer[256],
    length,
    data_size;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (ReadBlob(image,1,&data_size) != 1)
    ThrowBinaryException(CorruptImageError,"CorruptImage",image->filename);
  if (data_size > MaximumLZWBits)
    ThrowBinaryException(CorruptImageError,"CorruptImage",image->filename);
  if (ReadBlob(image,1,&length) != 1)
    ThrowBinaryException(CorruptImageError,"CorruptImage",image->filename);
  while (length != 0)
  {
    if (ReadBlob(image,length,buffer) != (ssize_t) length)
      ThrowBinaryException(CorruptImageError,"CorruptImage",image->filename);
    if (ReadBlob(image,1,&length) != 1)
      ThrowBinaryException(CorruptImageError,"CorruptImage",image->filename);
  }
  return(MagickTrue);
}

static Image *ReadGIFImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define BitSet(byte,bit)  (((byte) & (bit)) == (bit))
#define LSBFirstOrder(x,y)  (((y) << 8) | (x))
#define ThrowGIFException(exception,message) \
{ \
  if (profiles != (LinkedListInfo *) NULL) \
    profiles=DestroyLinkedList(profiles,DestroyGIFProfile); \
  if (global_colormap != (unsigned char *) NULL) \
    global_colormap=(unsigned char *) RelinquishMagickMemory(global_colormap); \
  if (meta_image != (Image *) NULL) \
    meta_image=DestroyImage(meta_image); \
  ThrowReaderException((exception),(message)); \
}

  Image
    *image,
    *meta_image;

  LinkedListInfo
    *profiles;

  MagickBooleanType
    status;

  ssize_t
    i;

  unsigned char
    *p;