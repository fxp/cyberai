// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 39/59

t double x,const double y,double *pixel,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o image_view: the image view.
%
%    o channel: the pixel channel to interpolate.
%
%    o method: the pixel color interpolation method.
%
%    o x,y: A double representing the current (x,y) position of the pixel.
%
%    o pixel: return the interpolated pixel here.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline void CatromWeights(const double x,double (*weights)[4])
{
  double
    alpha,
    beta,
    gamma;

  /*
    Nicolas Robidoux' 10 flops (4* + 5- + 1+) refactoring of the computation
    of the standard four 1D Catmull-Rom weights. The sampling location is
    assumed between the second and third input pixel locations, and x is the
    position relative to the second input pixel location. Formulas originally
    derived for the VIPS (Virtual Image Processing System) library.
  */
  alpha=(double) 1.0-x;
  beta=(double) (-0.5)*x*alpha;
  (*weights)[0]=alpha*beta;
  (*weights)[3]=x*beta;
  /*
    The following computation of the inner weights from the outer ones work
    for all Keys cubics.
  */
  gamma=(*weights)[3]-(*weights)[0];
  (*weights)[1]=alpha-(*weights)[0]+gamma;
  (*weights)[2]=x-(*weights)[3]-gamma;
}

static inline void SplineWeights(const double x,double (*weights)[4])
{
  double
    alpha,
    beta;

  /*
    Nicolas Robidoux' 12 flops (6* + 5- + 1+) refactoring of the computation
    of the standard four 1D cubic B-spline smoothing weights. The sampling
    location is assumed between the second and third input pixel locations,
    and x is the position relative to the second input pixel location.
  */
  alpha=(double) 1.0-x;
  (*weights)[3]=(double) (1.0/6.0)*x*x*x;
  (*weights)[0]=(double) (1.0/6.0)*alpha*alpha*alpha;
  beta=(*weights)[3]-(*weights)[0];
  (*weights)[1]=alpha-(*weights)[0]+beta;
  (*weights)[2]=x-(*weights)[3]-beta;
}

static inline double MeshInterpolate(const PointInfo *delta,const double p,
  const double x,const double y)
{
  return(delta->x*x+delta->y*y+(1.0-delta->x-delta->y)*p);
}

MagickExport MagickBooleanType InterpolatePixelChannel(
  const Image *magick_restrict image,const CacheView_ *image_view,
  const PixelChannel channel,const PixelInterpolateMethod method,const double x,
  const double y,double *pixel,ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  double
    alpha[16],
    gamma,
    pixels[16];

  MagickBooleanType
    status;

  PixelInterpolateMethod
    interpolate;

  PixelTrait
    traits;

  ssize_t
    i,
    x_offset,
    y_offset;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image_view != (CacheView *) NULL);
  *pixel=0.0;
  if ((channel < 0) || (channel >= MaxPixelChannels))
    ThrowBinaryException(OptionError,"NoSuchImageChannel",image->filename);
  traits=GetPixelChannelTraits(image,channel);
  x_offset=CastDoubleToSsizeT(floor(x));
  y_offset=CastDoubleToSsizeT(floor(y));
  interpolate=method;
  if (interpolate == UndefinedInterpolatePixel)
    interpolate=image->interpolate;
  status=MagickTrue;
  switch (interpolate)
  {
    case AverageInterpolatePixel:  /* nearest 4 neighbours */
    case Average9InterpolatePixel:  /* nearest 9 neighbours */
    case Average16InterpolatePixel:  /* nearest 16 neighbours */
    {
      ssize_t
        count;

      count=2;  /* size of the area to average - default nearest 4 */
      if (interpolate == Average9InterpolatePixel)
        {
          count=3;
          x_offset=CastDoubleToSsizeT(floor(x+0.5)-1.0);
          y_offset=CastDoubleToSsizeT(floor(y+0.5)-1.0);
        }
      else
        if (interpolate == Average16InterpolatePixel)
          {
            count=4;
            x_offset--;
            y_offset--;
          }
      p=GetCacheViewVirtualPixels(image_view,x_offset,y_offset,(size_t) count,
        (size_t) count,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      count*=count;  /* Number of pixels to average */
      if ((traits & BlendPixelTrait) == 0)
        for (i=0; i < (ssize_t) count; i++)
        {
          alpha[i]=1.0;
          pixels[i]=(double) p[i*(ssize_t) GetPixelChannels(image)+
            GetPixelChannelOffset(image,channel)];
        }
      else
        for (i=0; i < (ssize_t) count; i++)
        {
          alpha[i]=QuantumScale*(double) GetPixelAlpha(image,p+i*
            (ssize_t) GetPixelChannels(image));
          pixels[i]=alpha[i]*(double) p[i*(ssize_t) GetPixelChannels(image)+
            GetPixelChannelOffset(image,channel)];
        }
      for (i=0; i < (ssize_t) count; i++)
      {
        gamma=MagickSafeReciprocal(alpha[i])/count;
        *pixel+=gamma*pixels[i];
      }
      break;
    }
    case BilinearInterpolatePixel:
    default:
    {
      PointInfo
        delta,
        epsilon;