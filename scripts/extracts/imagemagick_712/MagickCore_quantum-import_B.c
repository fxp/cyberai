// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 2/33

t Image *image,CacheView *image_view,
%        QuantumInfo *quantum_info,const QuantumType quantum_type,
%        const unsigned char *magick_restrict pixels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o image_view: the image cache view.
%
%    o quantum_info: the quantum info.
%
%    o quantum_type: Declare which pixel components to transfer (red, green,
%      blue, opacity, RGB, or RGBA).
%
%    o pixels:  The pixel components are transferred from this buffer.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline Quantum PushColormapIndex(const Image *image,const size_t index,
  MagickBooleanType *range_exception)
{
  if (index < image->colors)
    return((Quantum) index);
  *range_exception=MagickTrue;
  return((Quantum) 0);
}

static inline const unsigned char *PushDoublePixel(QuantumInfo *quantum_info,
  const unsigned char *magick_restrict pixels,double *pixel)
{
  double
    *p;

  unsigned char
    quantum[8];

  if (quantum_info->endian == LSBEndian)
    {
      quantum[0]=(*pixels++);
      quantum[1]=(*pixels++);
      quantum[2]=(*pixels++);
      quantum[3]=(*pixels++);
      quantum[4]=(*pixels++);
      quantum[5]=(*pixels++);
      quantum[6]=(*pixels++);
      quantum[7]=(*pixels++);
    }
  else
    {
      quantum[7]=(*pixels++);
      quantum[6]=(*pixels++);
      quantum[5]=(*pixels++);
      quantum[4]=(*pixels++);
      quantum[3]=(*pixels++);
      quantum[2]=(*pixels++);
      quantum[1]=(*pixels++);
      quantum[0]=(*pixels++);
    }
  p=(double *) quantum;
  *pixel=(*p);
  *pixel-=quantum_info->minimum;
  *pixel*=quantum_info->scale;
  return(pixels);
}

static inline float ScaleFloatPixel(const QuantumInfo *quantum_info,
  const unsigned char *quantum)
{
  double
    pixel;

  pixel=(double) (*((float *) quantum));
  pixel-=quantum_info->minimum;
  pixel*=quantum_info->scale;
  if (pixel < (double) -FLT_MAX)
    return(-FLT_MAX);
  if (pixel > (double) FLT_MAX)
    return(FLT_MAX);
  return((float) pixel);
}

static inline const unsigned char *PushQuantumFloatPixel(
  const QuantumInfo *quantum_info,const unsigned char *magick_restrict pixels,
  float *pixel)
{
  unsigned char
    quantum[4];

  if (quantum_info->endian == LSBEndian)
    {
      quantum[0]=(*pixels++);
      quantum[1]=(*pixels++);
      quantum[2]=(*pixels++);
      quantum[3]=(*pixels++);
    }
  else
    {
      quantum[3]=(*pixels++);
      quantum[2]=(*pixels++);
      quantum[1]=(*pixels++);
      quantum[0]=(*pixels++);
    }
  *pixel=ScaleFloatPixel(quantum_info,quantum);
  return(pixels);
}

static inline const unsigned char *PushQuantumFloat24Pixel(
  const QuantumInfo *quantum_info,const unsigned char *magick_restrict pixels,
  float *pixel)
{
  unsigned char
    quantum[4];

  if (quantum_info->endian == LSBEndian)
    {
      quantum[0]=(*pixels++);
      quantum[1]=(*pixels++);
      quantum[2]=(*pixels++);
    }
  else
    {
      quantum[2]=(*pixels++);
      quantum[1]=(*pixels++);
      quantum[0]=(*pixels++);
    }
  if ((quantum[0] | quantum[1] | quantum[2]) == 0U)
    quantum[3]=0;
  else
    {
      unsigned char
        exponent,
        sign_bit;

      sign_bit=(quantum[2] & 0x80);
      exponent=(quantum[2] & 0x7F);
      if (exponent != 0)
        exponent=exponent-63+127;
      quantum[3]=sign_bit | (exponent >> 1);
      quantum[2]=((exponent & 1) << 7) | ((quantum[1] & 0xFE) >> 1);
      quantum[1]=((quantum[1] & 0x01) << 7) | ((quantum[0] & 0xFE) >> 1);
      quantum[0]=(quantum[0] & 0x01) << 7;
    }
  *pixel=ScaleFloatPixel(quantum_info,quantum);
  return(pixels);
}

static inline const unsigned char *PushQuantumPixel(QuantumInfo *quantum_info,
  const unsigned char *magick_restrict pixels,unsigned int *quantum)
{
  ssize_t
    i;

  size_t
    quantum_bits;

  *quantum=(QuantumAny) 0;
  for (i=(ssize_t) quantum_info->depth; i > 0L; )
  {
    if (quantum_info->state.bits == 0UL)
      {
        quantum_info->state.pixel=(*pixels++);
        quantum_info->state.bits=8UL;
      }
    quantum_bits=(size_t) i;
    if (quantum_bits > quantum_info->state.bits)
      quantum_bits=quantum_info->state.bits;
    i-=(ssize_t) quantum_bits;
    quantum_info->state.bits-=quantum_bits;
    if (quantum_bits < 64)
      *quantum=(unsigned int) (((MagickSizeType) *quantum << quantum_bits) |
        ((quantum_info->state.pixel >> quantum_info->state.bits) &~
        ((~0UL) << quantum_bits)));
  }
  return(pixels);
}

static inline const unsigned char *PushQuantumLongPixel(
  QuantumInfo *quantum_info,const unsigned char *magick_restrict pixels,
  unsigned int *quantum)
{
  ssize_t
    i;

  size_t
    quantum_bits;