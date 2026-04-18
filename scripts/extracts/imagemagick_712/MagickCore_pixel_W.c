// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 55/59



  fuzz=p->fuzz*p->fuzz+q->fuzz*q->fuzz;
  scale=1.0;
  distance=0.0;
  if ((p->alpha_trait != UndefinedPixelTrait) ||
      (q->alpha_trait != UndefinedPixelTrait))
    {
      /*
        Transparencies are involved - set alpha distance.
      */
      pixel=(p->alpha_trait != UndefinedPixelTrait ? p->alpha :
        (double) OpaqueAlpha)-(q->alpha_trait != UndefinedPixelTrait ?
        q->alpha : (double) OpaqueAlpha);
      distance=pixel*pixel;
      if (MagickSafeSignificantError(distance*distance,fuzz) != MagickFalse)
        return(MagickFalse);
      /*
        Generate a alpha scaling factor to generate a 4D cone on colorspace.
        If one color is transparent, distance has no color component.
      */
      if (p->alpha_trait != UndefinedPixelTrait)
        scale=(QuantumScale*p->alpha);
      if (q->alpha_trait != UndefinedPixelTrait)
        scale*=(QuantumScale*q->alpha);
      if (scale <= MagickEpsilon)
        return(MagickTrue);
    }
  /*
    CMYK create a CMY cube with a multi-dimensional cone toward black.
  */
  if (p->colorspace == CMYKColorspace)
    {
      pixel=p->black-q->black;
      distance+=pixel*pixel*scale;
      if (MagickSafeSignificantError(distance*distance,fuzz) != MagickFalse)
        return(MagickFalse);
      scale*=QuantumScale*((double) QuantumRange-(double) p->black);
      scale*=QuantumScale*((double) QuantumRange-(double) q->black);
    }
  /*
    RGB or CMY color cube.
  */
  distance*=3.0;  /* rescale appropriately */
  fuzz*=3.0;
  pixel=p->red-q->red;
  if (IsHueCompatibleColorspace(p->colorspace) != MagickFalse)
    {
      /*
        This calculates a arc distance for hue-- it should be a vector
        angle of 'S'/'W' length with 'L'/'B' forming appropriate cones.
        In other words this is a hack - Anthony.
      */
      if (fabs((double) pixel) > ((double) QuantumRange/2.0))
        pixel-=(double) QuantumRange;
      pixel*=2.0;
    }
  distance+=pixel*pixel*scale;
  if (MagickSafeSignificantError(distance,fuzz) != MagickFalse)
    return(MagickFalse);
  pixel=p->green-q->green;
  distance+=pixel*pixel*scale;
  if (MagickSafeSignificantError(distance,fuzz) != MagickFalse)
    return(MagickFalse);
  pixel=p->blue-q->blue;
  distance+=pixel*pixel*scale;
  if (MagickSafeSignificantError(distance,fuzz) != MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e s e t P i x e l C h a n n e l M a p                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetPixelChannelMap() defines the standard pixel component map.
%
%  The format of the ResetPixelChannelMap() method is:
%
%      MagickBooleanType ResetPixelChannelMap(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickPrivate MagickBooleanType ResetPixelChannelMap(Image *image,
  ExceptionInfo *exception)
{
  PixelTrait
    trait;

  ssize_t
    n;