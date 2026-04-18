// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 45/59



        PixelChannel channel = GetPixelChannelChannel(source,i);
        PixelTrait traits = GetPixelChannelTraits(source,channel);
        PixelTrait destination_traits=GetPixelChannelTraits(destination,
          channel);
        if ((traits == UndefinedPixelTrait) ||
            (destination_traits == UndefinedPixelTrait))
          continue;
        if (source->alpha_trait != BlendPixelTrait)
          for (j=0; j < 4; j++)
          {
            alpha[j]=1.0;
            pixels[j]=(double) p[j*(ssize_t) GetPixelChannels(source)+i];
          }
        else
          for (j=0; j < 4; j++)
          {
            alpha[j]=QuantumScale*(double) GetPixelAlpha(source,p+j*
              (ssize_t) GetPixelChannels(source));
            pixels[j]=(double) p[j*(ssize_t) GetPixelChannels(source)+i];
            if (channel != AlphaPixelChannel)
              pixels[j]*=alpha[j];
          }
        gamma=1.0;  /* number of pixels blended together (its variable) */
        for (j=0; j <= 1L; j++)
        {
          if ((y-y_offset) >= 0.75)
            {
              alpha[j]=alpha[j+2];  /* take right pixels */
              pixels[j]=pixels[j+2];
            }
          else
            if ((y-y_offset) > 0.25)
              {
                gamma=2.0;  /* blend both pixels in row */
                alpha[j]+=alpha[j+2];  /* add up alpha weights */
                pixels[j]+=pixels[j+2];
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
        SetPixelChannel(destination,channel,ClampToQuantum(gamma*pixels[0]),
          pixel);
      }
      break;
    }
    case CatromInterpolatePixel:
    {
      double
        cx[4],
        cy[4];

      p=GetCacheViewVirtualPixels(source_view,x_offset-1,y_offset-1,4,4,
        exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      for (i=0; i < (ssize_t) GetPixelChannels(source); i++)
      {
        ssize_t
          j;