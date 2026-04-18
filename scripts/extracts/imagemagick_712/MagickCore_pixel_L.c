// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 44/59



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
      p=GetCacheViewVirtualPixels(source_view,x_offset,y_offset,(size_t) count,
        (size_t) count,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      count*=count;  /* Number of pixels to average */
      for (i=0; i < (ssize_t) GetPixelChannels(source); i++)
      {
        double
          sum;

        ssize_t
          j;

        PixelChannel channel = GetPixelChannelChannel(source,i);
        PixelTrait traits = GetPixelChannelTraits(source,channel);
        PixelTrait destination_traits=GetPixelChannelTraits(destination,
          channel);
        if ((traits == UndefinedPixelTrait) ||
            (destination_traits == UndefinedPixelTrait))
          continue;
        for (j=0; j < (ssize_t) count; j++)
          pixels[j]=(double) p[j*(ssize_t) GetPixelChannels(source)+i];
        sum=0.0;
        if ((traits & BlendPixelTrait) == 0)
          {
            for (j=0; j < (ssize_t) count; j++)
              sum+=pixels[j];
            sum/=count;
            SetPixelChannel(destination,channel,ClampToQuantum(sum),pixel);
            continue;
          }
        for (j=0; j < (ssize_t) count; j++)
        {
          alpha[j]=QuantumScale*(double) GetPixelAlpha(source,p+j*
            (ssize_t) GetPixelChannels(source));
          pixels[j]*=alpha[j];
          gamma=MagickSafeReciprocal(alpha[j]);
          sum+=gamma*pixels[j];
        }
        sum/=count;
        SetPixelChannel(destination,channel,ClampToQuantum(sum),pixel);
      }
      break;
    }
    case BilinearInterpolatePixel:
    default:
    {
      p=GetCacheViewVirtualPixels(source_view,x_offset,y_offset,2,2,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      for (i=0; i < (ssize_t) GetPixelChannels(source); i++)
      {
        PointInfo
          delta,
          epsilon;

        PixelChannel channel = GetPixelChannelChannel(source,i);
        PixelTrait traits = GetPixelChannelTraits(source,channel);
        PixelTrait destination_traits=GetPixelChannelTraits(destination,
          channel);
        if ((traits == UndefinedPixelTrait) ||
            (destination_traits == UndefinedPixelTrait))
          continue;
        delta.x=x-x_offset;
        delta.y=y-y_offset;
        epsilon.x=1.0-delta.x;
        epsilon.y=1.0-delta.y;
        pixels[0]=(double) p[i];
        pixels[1]=(double) p[(ssize_t) GetPixelChannels(source)+i];
        pixels[2]=(double) p[2*(ssize_t) GetPixelChannels(source)+i];
        pixels[3]=(double) p[3*(ssize_t) GetPixelChannels(source)+i];
        if ((traits & BlendPixelTrait) == 0)
          {
            gamma=((epsilon.y*(epsilon.x+delta.x)+delta.y*(epsilon.x+delta.x)));
            gamma=MagickSafeReciprocal(gamma);
            SetPixelChannel(destination,channel,ClampToQuantum(gamma*(epsilon.y*
              (epsilon.x*pixels[0]+delta.x*pixels[1])+delta.y*(epsilon.x*
              pixels[2]+delta.x*pixels[3]))),pixel);
            continue;
          }
        alpha[0]=QuantumScale*(double) GetPixelAlpha(source,p);
        alpha[1]=QuantumScale*(double) GetPixelAlpha(source,p+
          GetPixelChannels(source));
        alpha[2]=QuantumScale*(double) GetPixelAlpha(source,p+2*
          GetPixelChannels(source));
        alpha[3]=QuantumScale*(double) GetPixelAlpha(source,p+3*
          GetPixelChannels(source));
        pixels[0]*=alpha[0];
        pixels[1]*=alpha[1];
        pixels[2]*=alpha[2];
        pixels[3]*=alpha[3];
        gamma=((epsilon.y*(epsilon.x*alpha[0]+delta.x*alpha[1])+delta.y*
          (epsilon.x*alpha[2]+delta.x*alpha[3])));
        gamma=MagickSafeReciprocal(gamma);
        SetPixelChannel(destination,channel,ClampToQuantum(gamma*(epsilon.y*
          (epsilon.x*pixels[0]+delta.x*pixels[1])+delta.y*(epsilon.x*pixels[2]+
          delta.x*pixels[3]))),pixel);
      }
      break;
    }
    case BlendInterpolatePixel:
    {
      p=GetCacheViewVirtualPixels(source_view,x_offset,y_offset,2,2,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      for (i=0; i < (ssize_t) GetPixelChannels(source); i++)
      {
        ssize_t
          j;