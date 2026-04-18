// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 52/59



      p=GetCacheViewVirtualPixels(image_view,x_offset,y_offset,2,2,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      delta.x=x-x_offset;
      delta.y=y-y_offset;
      luminance.x=GetPixelLuma(image,p)-(double)
        GetPixelLuma(image,p+3*GetPixelChannels(image));
      luminance.y=GetPixelLuma(image,p+GetPixelChannels(image))-(double)
        GetPixelLuma(image,p+2*GetPixelChannels(image));
      AlphaBlendPixelInfo(image,p,pixels+0,alpha+0);
      AlphaBlendPixelInfo(image,p+GetPixelChannels(image),pixels+1,alpha+1);
      AlphaBlendPixelInfo(image,p+2*GetPixelChannels(image),pixels+2,alpha+2);
      AlphaBlendPixelInfo(image,p+3*GetPixelChannels(image),pixels+3,alpha+3);
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
              pixel->red=gamma*MeshInterpolate(&delta,pixels[2].red,
                pixels[3].red,pixels[0].red);
              pixel->green=gamma*MeshInterpolate(&delta,pixels[2].green,
                pixels[3].green,pixels[0].green);
              pixel->blue=gamma*MeshInterpolate(&delta,pixels[2].blue,
                pixels[3].blue,pixels[0].blue);
              if (image->colorspace == CMYKColorspace)
                pixel->black=gamma*MeshInterpolate(&delta,pixels[2].black,
                  pixels[3].black,pixels[0].black);
              gamma=MeshInterpolate(&delta,1.0,1.0,1.0);
              pixel->alpha=gamma*MeshInterpolate(&delta,pixels[2].alpha,
                pixels[3].alpha,pixels[0].alpha);
            }
          else
            {
              /*
                Top-right triangle (pixel:1 , diagonal: 0-3).
              */
              delta.x=1.0-delta.x;
              gamma=MeshInterpolate(&delta,alpha[1],alpha[0],alpha[3]);
              gamma=MagickSafeReciprocal(gamma);
              pixel->red=gamma*MeshInterpolate(&delta,pixels[1].red,
                pixels[0].red,pixels[3].red);
              pixel->green=gamma*MeshInterpolate(&delta,pixels[1].green,
                pixels[0].green,pixels[3].green);
              pixel->blue=gamma*MeshInterpolate(&delta,pixels[1].blue,
                pixels[0].blue,pixels[3].blue);
              if (image->colorspace == CMYKColorspace)
                pixel->black=gamma*MeshInterpolate(&delta,pixels[1].black,
                  pixels[0].black,pixels[3].black);
              gamma=MeshInterpolate(&delta,1.0,1.0,1.0);
              pixel->alpha=gamma*MeshInterpolate(&delta,pixels[1].alpha,
                pixels[0].alpha,pixels[3].alpha);
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
              pixel->red=gamma*MeshInterpolate(&delta,pixels[0].red,
                pixels[1].red,pixels[2].red);
              pixel->green=gamma*MeshInterpolate(&delta,pixels[0].green,
                pixels[1].green,pixels[2].green);
              pixel->blue=gamma*MeshInterpolate(&delta,pixels[0].blue,
                pixels[1].blue,pixels[2].blue);
              if (image->colorspace == CMYKColorspace)
                pixel->black=gamma*MeshInterpolate(&delta,pixels[0].black,
                  pixels[1].black,pixels[2].black);
              gamma=MeshInterpolate(&delta,1.0,1.0,1.0);
              pixel->alpha=gamma*MeshInterpolate(&delta,pixels[0].alpha,
                pixels[1].alpha,pixels[2].alpha);
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
              pixel->red=gamma*MeshInterpolate(&delta,pixels[3].red,
                pixels[2].red,pixels[1].red);
              pixel->green=gamma*MeshInterpolate(&delta,pixels[3].green,
                pixels[2].green,pixels[1].green);
              pixel->blue=gamma*MeshInterpolate(&delta,pixels[3].blue,
                pixels[2].blue,pixels[1].blue);
              if (image->colorspace == CMYKColorspace)
                pixel->black=gamma*MeshInterpolate(&delta,pixels[3].black,
                  pixels[2].black,pixels[1].black);
              gamma=MeshInterpolate(&delta,1.0,1.0,1.0);
              pixel->alpha=gamma*MeshInterpola