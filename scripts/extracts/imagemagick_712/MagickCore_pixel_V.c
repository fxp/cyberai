// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 54/59



  /*
    MSE metric for comparing two pixels.
  */
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  { 
    double
      error;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait target_traits = GetPixelChannelTraits(target_image,channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((target_traits & UpdatePixelTrait) == 0))
      continue;
    if (channel == AlphaPixelChannel)
      error=(double) p[i]-(double) GetPixelChannel(target_image,channel,q);
    else
      error=alpha*(double) p[i]-target_alpha*
        GetPixelChannel(target_image,channel,q);
    if (MagickSafeSignificantError(error*error,fuzz) != MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I s F u z z y E q u i v a l e n c e P i x e l I n f o                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsFuzzyEquivalencePixelInfo() returns true if the distance between two
%  colors is less than the specified distance in a linear three (or four)
%  dimensional color space.
%
%  This implements the equivalent of:
%    fuzz < sqrt(color_distance^2 * u.a*v.a  + alpha_distance^2)
%
%  Which produces a multi-dimensional cone for that colorspace along the
%  transparency vector.
%
%  For example for an RGB:
%    color_distance^2  = ( (u.r-v.r)^2 + (u.g-v.g)^2 + (u.b-v.b)^2 ) / 3
%
%  See https://imagemagick.org/Usage/bugs/fuzz_distance/
%
%  Hue colorspace distances need more work.  Hue is not a distance, it is an
%  angle!
%
%  A check that q is in the same color space as p should be made and the
%  appropriate mapping made.  -- Anthony Thyssen  8 December 2010
%
%  The format of the IsFuzzyEquivalencePixelInfo method is:
%
%      MagickBooleanType IsFuzzyEquivalencePixelInfo(const PixelInfo *p,
%        const PixelInfo *q)
%
%  A description of each parameter follows:
%
%    o p: Pixel p.
%
%    o q: Pixel q.
%
*/
MagickExport MagickBooleanType IsFuzzyEquivalencePixelInfo(const PixelInfo *p,
  const PixelInfo *q)
{
  double
    distance,
    fuzz,
    pixel,
    scale;