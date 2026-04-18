// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 46/59



        PixelChannel channel = GetPixelChannelChannel(source,i);
        PixelTrait traits = GetPixelChannelTraits(source,channel);
        PixelTrait destination_traits=GetPixelChannelTraits(destination,
          channel);
        if ((traits == UndefinedPixelTrait) ||
            (destination_traits == UndefinedPixelTrait))
          continue;
        if ((traits & BlendPixelTrait) == 0)
          for (j=0; j < 16; j++)
          {
            alpha[j]=1.0;
            pixels[j]=(double) p[j*(ssize_t) GetPixelChannels(source)+i];
          }
        else
          for (j=0; j < 16; j++)
          {
            alpha[j]=QuantumScale*(double) GetPixelAlpha(source,p+j*
              (ssize_t) GetPixelChannels(source));
            pixels[j]=alpha[j]*(double)
              p[j*(ssize_t) GetPixelChannels(source)+i];
          }
        CatromWeights((double) (x-x_offset),&cx);
        CatromWeights((double) (y-y_offset),&cy);
        gamma=((traits & BlendPixelTrait) ? (double) (1.0) :
          MagickSafeReciprocal(cy[0]*(cx[0]*alpha[0]+cx[1]*alpha[1]+cx[2]*
          alpha[2]+cx[3]*alpha[3])+cy[1]*(cx[0]*alpha[4]+cx[1]*alpha[5]+cx[2]*
          alpha[6]+cx[3]*alpha[7])+cy[2]*(cx[0]*alpha[8]+cx[1]*alpha[9]+cx[2]*
          alpha[10]+cx[3]*alpha[11])+cy[3]*(cx[0]*alpha[12]+cx[1]*alpha[13]+
          cx[2]*alpha[14]+cx[3]*alpha[15])));
        SetPixelChannel(destination,channel,ClampToQuantum(gamma*(cy[0]*(cx[0]*
          pixels[0]+cx[1]*pixels[1]+cx[2]*pixels[2]+cx[3]*pixels[3])+cy[1]*
          (cx[0]*pixels[4]+cx[1]*pixels[5]+cx[2]*pixels[6]+cx[3]*pixels[7])+
          cy[2]*(cx[0]*pixels[8]+cx[1]*pixels[9]+cx[2]*pixels[10]+cx[3]*
          pixels[11])+cy[3]*(cx[0]*pixels[12]+cx[1]*pixels[13]+cx[2]*
          pixels[14]+cx[3]*pixels[15]))),pixel);
      }
      break;
    }
    case IntegerInterpolatePixel:
    {
      p=GetCacheViewVirtualPixels(source_view,x_offset,y_offset,1,1,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      for (i=0; i < (ssize_t) GetPixelChannels(source); i++)
      {
        PixelChannel channel = GetPixelChannelChannel(source,i);
        PixelTrait traits = GetPixelChannelTraits(source,channel);
        PixelTrait destination_traits=GetPixelChannelTraits(destination,
          channel);
        if ((traits == UndefinedPixelTrait) ||
            (destination_traits == UndefinedPixelTrait))
          continue;
        SetPixelChannel(destination,channel,p[i],pixel);
      }
      break;
    }
    case NearestInterpolatePixel:
    {
      x_offset=CastDoubleToSsizeT(floor(x+0.5));
      y_offset=CastDoubleToSsizeT(floor(y+0.5));
      p=GetCacheViewVirtualPixels(source_view,x_offset,y_offset,1,1,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      for (i=0; i < (ssize_t) GetPixelChannels(source); i++)
      {
        PixelChannel channel = GetPixelChannelChannel(source,i);
        PixelTrait traits = GetPixelChannelTraits(source,channel);
        PixelTrait destination_traits=GetPixelChannelTraits(destination,
          channel);
        if ((traits == UndefinedPixelTrait) ||
            (destination_traits == UndefinedPixelTrait))
          continue;
        SetPixelChannel(destination,channel,p[i],pixel);
      }
      break;
    }
    case MeshInterpolatePixel:
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
          luminance;