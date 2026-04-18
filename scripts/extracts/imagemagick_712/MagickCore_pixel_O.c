// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 47/59



        PixelChannel channel = GetPixelChannelChannel(source,i);
        PixelTrait traits = GetPixelChannelTraits(source,channel);
        PixelTrait destination_traits=GetPixelChannelTraits(destination,
          channel);
        if ((traits == UndefinedPixelTrait) ||
            (destination_traits == UndefinedPixelTrait))
          continue;
        pixels[0]=(double) p[i];
        pixels[1]=(double) p[(ssize_t) GetPixelChannels(source)+i];
        pixels[2]=(double) p[2*(ssize_t) GetPixelChannels(source)+i];
        pixels[3]=(double) p[3*(ssize_t) GetPixelChannels(source)+i];
        if ((traits & BlendPixelTrait) == 0)
          {
            alpha[0]=1.0;
            alpha[1]=1.0;
            alpha[2]=1.0;
            alpha[3]=1.0;
          }
        else
          {
            alpha[0]=QuantumScale*(double) GetPixelAlpha(source,p);
            alpha[1]=QuantumScale*(double) GetPixelAlpha(source,p+
              GetPixelChannels(source));
            alpha[2]=QuantumScale*(double) GetPixelAlpha(source,p+2*
              GetPixelChannels(source));
            alpha[3]=QuantumScale*(double) GetPixelAlpha(source,p+3*
              GetPixelChannels(source));
          }
        delta.x=x-x_offset;
        delta.y=y-y_offset;
        luminance.x=fabs((double) (GetPixelLuma(source,p)-
          GetPixelLuma(source,p+3*GetPixelChannels(source))));
        luminance.y=fabs((double) (GetPixelLuma(source,p+
          GetPixelChannels(source))-GetPixelLuma(source,p+2*
          GetPixelChannels(source))));
        if (luminance.x < luminance.y)
          {
            /*
              Diagonal 0-3 NW-SE.
            */
            if (delta.x <= delta.y)
              {
                /*
                  Bottom-left triangle (pixel: 2, diagonal: 0-3).
                */
                delta.y=1.0-delta.y;
                gamma=MeshInterpolate(&delta,alpha[2],alpha[3],alpha[0]);
                gamma=MagickSafeReciprocal(gamma);
                SetPixelChannel(destination,channel,ClampToQuantum(gamma*
                  MeshInterpolate(&delta,pixels[2],pixels[3],pixels[0])),pixel);
              }
            else
              {
                /*
                  Top-right triangle (pixel: 1, diagonal: 0-3).
                */
                delta.x=1.0-delta.x;
                gamma=MeshInterpolate(&delta,alpha[1],alpha[0],alpha[3]);
                gamma=MagickSafeReciprocal(gamma);
                SetPixelChannel(destination,channel,ClampToQuantum(gamma*
                  MeshInterpolate(&delta,pixels[1],pixels[0],pixels[3])),pixel);
              }
          }
        else
          {
            /*
              Diagonal 1-2 NE-SW.
            */
            if (delta.x <= (1.0-delta.y))
              {
                /*
                  Top-left triangle (pixel: 0, diagonal: 1-2).
                */
                gamma=MeshInterpolate(&delta,alpha[0],alpha[1],alpha[2]);
                gamma=MagickSafeReciprocal(gamma);
                SetPixelChannel(destination,channel,ClampToQuantum(gamma*
                  MeshInterpolate(&delta,pixels[0],pixels[1],pixels[2])),pixel);
              }
            else
              {
                /*
                  Bottom-right triangle (pixel: 3, diagonal: 1-2).
                */
                delta.x=1.0-delta.x;
                delta.y=1.0-delta.y;
                gamma=MeshInterpolate(&delta,alpha[3],alpha[2],alpha[1]);
                gamma=MagickSafeReciprocal(gamma);
                SetPixelChannel(destination,channel,ClampToQuantum(gamma*
                  MeshInterpolate(&delta,pixels[3],pixels[2],pixels[1])),pixel);
              }
          }
      }
      break;
    }
    case SplineInterpolatePixel:
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