// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 40/59



      p=GetCacheViewVirtualPixels(image_view,x_offset,y_offset,2,2,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      if ((traits & BlendPixelTrait) == 0)
        for (i=0; i < 4; i++)
        {
          alpha[i]=1.0;
          pixels[i]=(double) p[i*(ssize_t) GetPixelChannels(image)+
            GetPixelChannelOffset(image,channel)];
        }
      else
        for (i=0; i < 4; i++)
        {
          alpha[i]=QuantumScale*(double) GetPixelAlpha(image,p+i*
            (ssize_t) GetPixelChannels(image));
          pixels[i]=alpha[i]*(double) p[i*(ssize_t) GetPixelChannels(image)+
            GetPixelChannelOffset(image,channel)];
        }
      delta.x=x-x_offset;
      delta.y=y-y_offset;
      epsilon.x=1.0-delta.x;
      epsilon.y=1.0-delta.y;
      gamma=((epsilon.y*(epsilon.x*alpha[0]+delta.x*alpha[1])+delta.y*
        (epsilon.x*alpha[2]+delta.x*alpha[3])));
      gamma=MagickSafeReciprocal(gamma);
      *pixel=gamma*(epsilon.y*(epsilon.x*pixels[0]+delta.x*pixels[1])+delta.y*
        (epsilon.x*pixels[2]+delta.x*pixels[3]));
      break;
    }
    case BlendInterpolatePixel:
    {
      p=GetCacheViewVirtualPixels(image_view,x_offset,y_offset,2,2,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      if ((traits & BlendPixelTrait) == 0)
        for (i=0; i < 4; i++)
        {
          alpha[i]=1.0;
          pixels[i]=(MagickRealType) p[i*(ssize_t) GetPixelChannels(image)+
            GetPixelChannelOffset(image,channel)];
        }
      else
        for (i=0; i < 4; i++)
        {
          alpha[i]=QuantumScale*(double) GetPixelAlpha(image,p+i*
            (ssize_t) GetPixelChannels(image));
          pixels[i]=alpha[i]*(double) p[i*(ssize_t) GetPixelChannels(image)+
            GetPixelChannelOffset(image,channel)];
        }
      gamma=1.0;  /* number of pixels blended together (its variable) */
      for (i=0; i <= 1L; i++) {
        if ((y-y_offset) >= 0.75)
          {
            alpha[i]=alpha[i+2];  /* take right pixels */
            pixels[i]=pixels[i+2];
          }
        else
          if ((y-y_offset) > 0.25)
            {
              gamma=2.0;  /* blend both pixels in row */
              alpha[i]+=alpha[i+2];  /* add up alpha weights */
              pixels[i]+=pixels[i+2];
            }
      }
      if ((x-x_offset) >= 0.75)
        {
          alpha[0]=alpha[1];  /* take bottom row blend */
          pixels[0]=pixels[1];
        }
      else
        if ((x-x_offset) > 0.25)
          {
            gamma*=2.0;  /* blend both rows */
            alpha[0]+=alpha[1];  /* add up alpha weights */
            pixels[0]+=pixels[1];
          }
      if (channel != AlphaPixelChannel)
        gamma=MagickSafeReciprocal(alpha[0]);  /* (color) 1/alpha_weights */
      else
        gamma=MagickSafeReciprocal(gamma);  /* (alpha) 1/number_of_pixels */
      *pixel=gamma*pixels[0];
      break;
    }
    case CatromInterpolatePixel:
    {
      double
        cx[4],
        cy[4];