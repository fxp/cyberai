// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 43/59



      p=GetCacheViewVirtualPixels(image_view,x_offset-1,y_offset-1,4,4,
        exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      if ((traits & BlendPixelTrait) == 0)
        for (i=0; i < 16; i++)
        {
          alpha[i]=1.0;
          pixels[i]=(double) p[i*(ssize_t) GetPixelChannels(image)+
            GetPixelChannelOffset(image,channel)];
        }
      else
        for (i=0; i < 16; i++)
        {
          alpha[i]=QuantumScale*(double) GetPixelAlpha(image,p+i*
            (ssize_t) GetPixelChannels(image));
          pixels[i]=alpha[i]*(double) p[i*(ssize_t) GetPixelChannels(image)+
            GetPixelChannelOffset(image,channel)];
        }
      SplineWeights((double) (x-x_offset),&cx);
      SplineWeights((double) (y-y_offset),&cy);
      gamma=(channel == AlphaPixelChannel ? (double) 1.0 :
        MagickSafeReciprocal(cy[0]*(cx[0]*alpha[0]+cx[1]*alpha[1]+cx[2]*
        alpha[2]+cx[3]*alpha[3])+cy[1]*(cx[0]*alpha[4]+cx[1]*alpha[5]+cx[2]*
        alpha[6]+cx[3]*alpha[7])+cy[2]*(cx[0]*alpha[8]+cx[1]*alpha[9]+cx[2]*
        alpha[10]+cx[3]*alpha[11])+cy[3]*(cx[0]*alpha[12]+cx[1]*alpha[13]+
        cx[2]*alpha[14]+cx[3]*alpha[15])));
      *pixel=gamma*(cy[0]*(cx[0]*pixels[0]+cx[1]*pixels[1]+cx[2]*pixels[2]+
        cx[3]*pixels[3])+cy[1]*(cx[0]*pixels[4]+cx[1]*pixels[5]+cx[2]*
        pixels[6]+cx[3]*pixels[7])+cy[2]*(cx[0]*pixels[8]+cx[1]*pixels[9]+
        cx[2]*pixels[10]+cx[3]*pixels[11])+cy[3]*(cx[0]*pixels[12]+cx[1]*
        pixels[13]+cx[2]*pixels[14]+cx[3]*pixels[15]));
      break;
    }
  }
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n t e r p o l a t e P i x e l C h a n n e l s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpolatePixelChannels() applies a pixel interpolation method between a
%  floating point coordinate and the pixels surrounding that coordinate.  No
%  pixel area resampling, or scaling of the result is performed.
%
%  Interpolation is restricted to just the current channel setting of the
%  destination image into which the color is to be stored
%
%  The format of the InterpolatePixelChannels method is:
%
%      MagickBooleanType InterpolatePixelChannels(
%        const Image *magick_restrict source,const CacheView *source_view,
%        const Image *magick_restrict destination,
%        const PixelInterpolateMethod method,const double x,const double y,
%        Quantum *pixel,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o source: the source.
%
%    o source_view: the source view.
%
%    o destination: the destination image, for the interpolated color
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
MagickExport MagickBooleanType InterpolatePixelChannels(
  const Image *magick_restrict source,const CacheView_ *source_view,
  const Image *magick_restrict destination,const PixelInterpolateMethod method,
  const double x,const double y,Quantum *pixel,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  double
    alpha[16],
    gamma,
    pixels[16];

  const Quantum
    *magick_restrict p;

  ssize_t
    i;

  ssize_t
    x_offset,
    y_offset;

  PixelInterpolateMethod
    interpolate;

  assert(source != (Image *) NULL);
  assert(source->signature == MagickCoreSignature);
  assert(source_view != (CacheView *) NULL);
  status=MagickTrue;
  x_offset=CastDoubleToSsizeT(floor(x));
  y_offset=CastDoubleToSsizeT(floor(y));
  interpolate=method;
  if (interpolate == UndefinedInterpolatePixel)
    interpolate=source->interpolate;
  switch (interpolate)
  {
    case AverageInterpolatePixel:  /* nearest 4 neighbours */
    case Average9InterpolatePixel:  /* nearest 9 neighbours */
    case Average16InterpolatePixel:  /* nearest 16 neighbours */
    {
      ssize_t
        count;