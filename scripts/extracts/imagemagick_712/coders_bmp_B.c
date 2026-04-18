// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 2/24



  unsigned int
    compression,
    image_size,
    x_pixels,
    y_pixels,
    number_colors,
    red_mask,
    green_mask,
    blue_mask,
    alpha_mask,
    colors_important;

  long
    colorspace;

  PrimaryInfo
    red_primary,
    green_primary,
    blue_primary,
    gamma_scale;
} BMPInfo;

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteBMPImage(const ImageInfo *,Image *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e c o d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DecodeImage unpacks the packed image pixels into runlength-encoded pixel
%  packets.
%
%  The format of the DecodeImage method is:
%
%      MagickBooleanType DecodeImage(Image *image,const size_t compression,
%        unsigned char *pixels,const size_t number_pixels)
%
%  A description of each parameter follows:
%
%    o image: the address of a structure of type Image.
%
%    o compression:  Zero means uncompressed.  A value of 1 means the
%      compressed pixels are runlength encoded for a 256-color bitmap.
%      A value of 2 means a 16-color bitmap.  A value of 3 means bitfields
%      encoding.
%
%    o pixels:  The address of a byte (8 bits) array of pixel data created by
%      the decoding process.
%
%    o number_pixels:  The number of pixels.
%
*/
static MagickBooleanType DecodeImage(Image *image,const size_t compression,
  unsigned char *pixels,const size_t number_pixels)
{
  int
    byte,
    count;

  ssize_t
    i,
    x,
    y;

  unsigned char
    *p,
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(pixels != (unsigned char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  (void) memset(pixels,0,number_pixels*sizeof(*pixels));
  byte=0;
  x=0;
  p=pixels;
  q=pixels+number_pixels;
  for (y=0; y < (ssize_t) image->rows; )
  {
    MagickBooleanType
      status;