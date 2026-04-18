// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/webp.c
// Segment 2/11

of each parameter follows:
%
%    o magick: compare image format pattern against these bytes.
%
%    o length: Specifies the length of the magick string.
%
*/
static MagickBooleanType IsWEBP(const unsigned char *magick,const size_t length)
{
  if (length < 12)
    return(MagickFalse);
  if (LocaleNCompare((const char *) magick+8,"WEBP",4) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

#if defined(MAGICKCORE_WEBP_DELEGATE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d W E B P I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadWEBPImage() reads an image in the WebP image format.
%
%  The format of the ReadWEBPImage method is:
%
%      Image *ReadWEBPImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline uint32_t ReadWebPLSBWord(
  const unsigned char *magick_restrict data)
{
  const unsigned char
    *p;

  uint32_t
    value;

  p=data;
  value=(uint32_t) (*p++);
  value|=((uint32_t) (*p++)) << 8;
  value|=((uint32_t) (*p++)) << 16;
  value|=((uint32_t) (*p++)) << 24;
  return(value);
}

static inline void WriteWebPLSBWord(unsigned char *magick_restrict data,
  const size_t value)
{
  unsigned char
    *p;

  p=data;
  *(p++)=(unsigned char) value;
  *(p++)=(unsigned char) (value >> 8);
  *(p++)=(unsigned char) (value >> 16);
  *(p++)=(unsigned char) (value >> 24);
}

static MagickBooleanType IsWEBPImageLossless(const unsigned char *stream,
  const size_t length)
{
#define VP8_CHUNK_INDEX  15
#define LOSSLESS_FLAG  'L'
#define EXTENDED_HEADER  'X'
#define VP8_CHUNK_HEADER  "VP8"
#define VP8_CHUNK_HEADER_SIZE  3
#define RIFF_HEADER_SIZE  12
#define VP8X_CHUNK_SIZE  10
#define TAG_SIZE  4
#define CHUNK_SIZE_BYTES  4
#define CHUNK_HEADER_SIZE  8
#define MAX_CHUNK_PAYLOAD  (~0U-CHUNK_HEADER_SIZE-1)

  size_t
    offset;

  /*
    Read simple header.
  */
  if (length <= VP8_CHUNK_INDEX)
    return(MagickFalse);
  if (stream[VP8_CHUNK_INDEX] != EXTENDED_HEADER)
    return(stream[VP8_CHUNK_INDEX] == LOSSLESS_FLAG ? MagickTrue : MagickFalse);
  /*
    Read extended header.
  */
  offset=RIFF_HEADER_SIZE+TAG_SIZE+CHUNK_SIZE_BYTES+VP8X_CHUNK_SIZE;
  while (offset <= (length-TAG_SIZE-TAG_SIZE-4))
  {
    uint32_t
      chunk_size,
      chunk_size_pad;

    chunk_size=ReadWebPLSBWord(stream+offset+TAG_SIZE);
    if (chunk_size > MAX_CHUNK_PAYLOAD)
      break;
    chunk_size_pad=(CHUNK_HEADER_SIZE+chunk_size+1) & (unsigned int) ~1;
    if (memcmp(stream+offset,VP8_CHUNK_HEADER,VP8_CHUNK_HEADER_SIZE) == 0)
      return(*(stream+offset+VP8_CHUNK_HEADER_SIZE) == LOSSLESS_FLAG ?
        MagickTrue : MagickFalse);
    offset+=chunk_size_pad;
  }
  return(MagickFalse);
}

static int FillBasicWEBPInfo(Image *image,const uint8_t *stream,size_t length,
  WebPDecoderConfig *configure)
{
  int
    webp_status;

  WebPBitstreamFeatures
    *magick_restrict features = &configure->input;

  webp_status=(int) WebPGetFeatures(stream,length,features);
  if (webp_status != VP8_STATUS_OK)
    return(webp_status);
  image->columns=(size_t) features->width;
  image->rows=(size_t) features->height;
  image->depth=8;
  image->alpha_trait=features->has_alpha != 0 ? BlendPixelTrait :
    UndefinedPixelTrait;
  return(webp_status);
}

static int ReadSingleWEBPImage(const ImageInfo *image_info,Image *image,
  const uint8_t *stream,size_t length,WebPDecoderConfig *configure,
  ExceptionInfo *exception,MagickBooleanType is_first)
{
  int
    webp_status;

  MagickBooleanType
    status;

  size_t
    canvas_width,
    canvas_height,
    image_width,
    image_height;

  ssize_t
    x_offset,
    y_offset,
    y;

  unsigned char
    *p;

  WebPDecBuffer
    *magick_restrict webp_image;