// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 41/59



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
      CatromWeights((double) (x-x_offset),&cx);
      CatromWeights((double) (y-y_offset),&cy);
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
    case IntegerInterpolatePixel:
    {
      p=GetCacheViewVirtualPixels(image_view,x_offset,y_offset,1,1,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      *pixel=(double) GetPixelChannel(image,channel,p);
      break;
    }
    case NearestInterpolatePixel:
    {
      x_offset=CastDoubleToSsizeT(floor(x+0.5));
      y_offset=CastDoubleToSsizeT(floor(y+0.5));
      p=GetCacheViewVirtualPixels(image_view,x_offset,y_offset,1,1,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      *pixel=(double) GetPixelChannel(image,channel,p);
      break;
    }
    case MeshInterpolatePixel:
    {
      PointInfo
        delta,
        luminance;