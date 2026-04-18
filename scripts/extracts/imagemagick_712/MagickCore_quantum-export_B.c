// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-export.c
// Segment 2/30

,CacheView *image_view,
%        QuantumInfo *quantum_info,const QuantumType quantum_type,
%        unsigned char *magick_restrict pixels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o image_view: the image cache view.
%
%    o quantum_info: the quantum info.
%
%    o quantum_type: Declare which pixel components to transfer (RGB, RGBA,
%      etc).
%
%    o pixels:  The components are transferred to this buffer.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline unsigned char *PopQuantumDoublePixel(QuantumInfo *quantum_info,
  const double pixel,unsigned char *magick_restrict pixels)
{
  double
    *p;

  unsigned char
    quantum[8];

  (void) memset(quantum,0,sizeof(quantum));
  p=(double *) quantum;
  *p=(double) (pixel*quantum_info->state.inverse_scale+quantum_info->minimum);
  if (quantum_info->endian == LSBEndian)
    {
      *pixels++=quantum[0];
      *pixels++=quantum[1];
      *pixels++=quantum[2];
      *pixels++=quantum[3];
      *pixels++=quantum[4];
      *pixels++=quantum[5];
      *pixels++=quantum[6];
      *pixels++=quantum[7];
      return(pixels);
    }
  *pixels++=quantum[7];
  *pixels++=quantum[6];
  *pixels++=quantum[5];
  *pixels++=quantum[4];
  *pixels++=quantum[3];
  *pixels++=quantum[2];
  *pixels++=quantum[1];
  *pixels++=quantum[0];
  return(pixels);
}

static inline unsigned char *PopQuantumFloatPixel(QuantumInfo *quantum_info,
  const float pixel,unsigned char *magick_restrict pixels)
{
  float
    *p;

  unsigned char
    quantum[4];

  (void) memset(quantum,0,sizeof(quantum));
  p=(float *) quantum;
  *p=(float) ((double) pixel*quantum_info->state.inverse_scale+
    quantum_info->minimum);
  if (quantum_info->endian == LSBEndian)
    {
      *pixels++=quantum[0];
      *pixels++=quantum[1];
      *pixels++=quantum[2];
      *pixels++=quantum[3];
      return(pixels);
    }
  *pixels++=quantum[3];
  *pixels++=quantum[2];
  *pixels++=quantum[1];
  *pixels++=quantum[0];
  return(pixels);
}

static inline unsigned char *PopQuantumPixel(QuantumInfo *quantum_info,
  const QuantumAny pixel,unsigned char *magick_restrict pixels)
{
  ssize_t
    i;

  size_t
    quantum_bits;

  if (quantum_info->state.bits == 0UL)
    quantum_info->state.bits=8U;
  for (i=(ssize_t) quantum_info->depth; i > 0L; )
  {
    quantum_bits=(size_t) i;
    if (quantum_bits > quantum_info->state.bits)
      quantum_bits=quantum_info->state.bits;
    i-=(ssize_t) quantum_bits;
    if (i < 0)
      i=0;
    if (quantum_info->state.bits == 8UL)
      *pixels='\0';
    quantum_info->state.bits-=quantum_bits;
    *pixels|=(((pixel >> i) &~ (((QuantumAny) ~0UL) << quantum_bits)) <<
      quantum_info->state.bits);
    if (quantum_info->state.bits == 0UL)
      {
        pixels++;
        quantum_info->state.bits=8UL;
      }
  }
  return(pixels);
}

static inline unsigned char *PopQuantumLongPixel(QuantumInfo *quantum_info,
  const size_t pixel,unsigned char *magick_restrict pixels)
{
  ssize_t
    i;

  size_t
    quantum_bits;

  if (quantum_info->state.bits == 0U)
    quantum_info->state.bits=32UL;
  for (i=(ssize_t) quantum_info->depth; i > 0; )
  {
    quantum_bits=(size_t) i;
    if (quantum_bits > quantum_info->state.bits)
      quantum_bits=quantum_info->state.bits;
    quantum_info->state.pixel|=(((pixel >> ((ssize_t) quantum_info->depth-i)) &
      quantum_info->state.mask[quantum_bits]) << (32U-
        quantum_info->state.bits));
    i-=(ssize_t) quantum_bits;
    quantum_info->state.bits-=quantum_bits;
    if (quantum_info->state.bits == 0U)
      {
        pixels=PopLongPixel(quantum_info->endian,quantum_info->state.pixel,
          pixels);
        quantum_info->state.pixel=0U;
        quantum_info->state.bits=32U;
      }
  }
  return(pixels);
}

static void ExportPixelChannel(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const Quantum *magick_restrict p,
  unsigned char *magick_restrict q,PixelChannel channel)
{
  QuantumAny
    range;

  ssize_t
    x;

  p+=(ptrdiff_t) image->channel_map[channel].offset;
  switch (quantum_info->depth)
  {
    case 8:
    {
      unsigned char
        pixel;

      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=ScaleQuantumToChar(*p);
        q=PopCharPixel(pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
    case 16:
    {
      unsigned short
        pixel;