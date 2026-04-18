// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 42/59



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
      luminance.x=GetPixelLuma(image,p)-(double)
        GetPixelLuma(image,p+3*GetPixelChannels(image));
      luminance.y=GetPixelLuma(image,p+GetPixelChannels(image))-(double)
        GetPixelLuma(image,p+2*GetPixelChannels(image));
      if (fabs((double) luminance.x) < fabs((double) luminance.y))
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
              *pixel=gamma*MeshInterpolate(&delta,pixels[2],pixels[3],
                pixels[0]);
            }
          else
            {
              /*
                Top-right triangle (pixel: 1, diagonal: 0-3).
              */
              delta.x=1.0-delta.x;
              gamma=MeshInterpolate(&delta,alpha[1],alpha[0],alpha[3]);
              gamma=MagickSafeReciprocal(gamma);
              *pixel=gamma*MeshInterpolate(&delta,pixels[1],pixels[0],
                pixels[3]);
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
              *pixel=gamma*MeshInterpolate(&delta,pixels[0],pixels[1],
                pixels[2]);
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
              *pixel=gamma*MeshInterpolate(&delta,pixels[3],pixels[2],
                pixels[1]);
            }
        }
      break;
    }
    case SplineInterpolatePixel:
    {
      double
        cx[4],
        cy[4];