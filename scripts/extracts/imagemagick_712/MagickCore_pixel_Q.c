// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 49/59



  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image_view != (CacheView *) NULL);
  status=MagickTrue;
  x_offset=CastDoubleToSsizeT(floor(x));
  y_offset=CastDoubleToSsizeT(floor(y));
  interpolate=method;
  if (interpolate == UndefinedInterpolatePixel)
    interpolate=image->interpolate;
  GetPixelInfoPixel(image,(const Quantum *) NULL,pixel);
  (void) memset(&pixels,0,sizeof(pixels));
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
      pixel->red=0.0;
      pixel->green=0.0;
      pixel->blue=0.0;
      pixel->black=0.0;
      pixel->alpha=0.0;
      count*=count;  /* number of pixels - square of size */
      for (i=0; i < (ssize_t) count; i++)
      {
        AlphaBlendPixelInfo(image,p,pixels,alpha);
        gamma=MagickSafeReciprocal(alpha[0]);
        pixel->red+=gamma*pixels[0].red;
        pixel->green+=gamma*pixels[0].green;
        pixel->blue+=gamma*pixels[0].blue;
        pixel->black+=gamma*pixels[0].black;
        pixel->alpha+=pixels[0].alpha;
        p+=(ptrdiff_t) GetPixelChannels(image);
      }
      gamma=1.0/count;   /* average weighting of each pixel in area */
      pixel->red*=gamma;
      pixel->green*=gamma;
      pixel->blue*=gamma;
      pixel->black*=gamma;
      pixel->alpha*=gamma;
      break;
    }
    case BackgroundInterpolatePixel:
    {
      *pixel=image->background_color;  /* Copy PixelInfo Structure  */
      break;
    }
    case BilinearInterpolatePixel:
    default:
    {
      PointInfo
        delta,
        epsilon;