// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/compress.c
// Segment 8/11

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L Z W E n c o d e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LZWEncodeImage() compresses an image via LZW-coding specific to Postscript
%  Level II or Portable Document Format.
%
%  The format of the LZWEncodeImage method is:
%
%      MagickBooleanType LZWEncodeImage(Image *image,const size_t length,
%        unsigned char *magick_restrict pixels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o length:  A value that specifies the number of pixels to compress.
%
%    o pixels: the address of an unsigned array of characters containing the
%      pixels to compress.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType LZWEncodeImage(Image *image,const size_t length,
  unsigned char *magick_restrict pixels,ExceptionInfo *exception)
{
#define LZWClr  256UL  /* Clear Table Marker */
#define LZWEod  257UL  /* End of Data marker */
#define OutputCode(code) \
{ \
    accumulator+=code << (32-code_width-number_bits); \
    number_bits+=code_width; \
    while (number_bits >= 8) \
    { \
      (void) WriteBlobByte(image,(unsigned char) (accumulator >> 24)); \
      accumulator=accumulator << 8; \
      number_bits-=8; \
    } \
}

  typedef struct _TableType
  {
    ssize_t
      prefix,
      suffix,
      next;
  } TableType;

  ssize_t
    i;

  size_t
    accumulator,
    number_bits,
    code_width,
    last_code,
    next_index;

  ssize_t
    index;

  TableType
    *table;