// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 50/59



      p=GetCacheViewVirtualPixels(image_view,x_offset,y_offset,2,2,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      for (i=0; i < 4L; i++)
        AlphaBlendPixelInfo(image,p+i*(ssize_t) GetPixelChannels(image),
          pixels+i,alpha+i);
      delta.x=x-x_offset;
      delta.y=y-y_offset;
      epsilon.x=1.0-delta.x;
      epsilon.y=1.0-delta.y;
      gamma=((epsilon.y*(epsilon.x*alpha[0]+delta.x*alpha[1])+delta.y*
        (epsilon.x*alpha[2]+delta.x*alpha[3])));
      gamma=MagickSafeReciprocal(gamma);
      pixel->red=gamma*(epsilon.y*(epsilon.x*pixels[0].red+delta.x*
        pixels[1].red)+delta.y*(epsilon.x*pixels[2].red+delta.x*pixels[3].red));
      pixel->green=gamma*(epsilon.y*(epsilon.x*pixels[0].green+delta.x*
        pixels[1].green)+delta.y*(epsilon.x*pixels[2].green+delta.x*
        pixels[3].green));
      pixel->blue=gamma*(epsilon.y*(epsilon.x*pixels[0].blue+delta.x*
        pixels[1].blue)+delta.y*(epsilon.x*pixels[2].blue+delta.x*
        pixels[3].blue));
      if (image->colorspace == CMYKColorspace)
        pixel->black=gamma*(epsilon.y*(epsilon.x*pixels[0].black+delta.x*
          pixels[1].black)+delta.y*(epsilon.x*pixels[2].black+delta.x*
          pixels[3].black));
      gamma=((epsilon.y*(epsilon.x+delta.x)+delta.y*(epsilon.x+delta.x)));
      gamma=MagickSafeReciprocal(gamma);
      pixel->alpha=gamma*(epsilon.y*(epsilon.x*pixels[0].alpha+delta.x*
        pixels[1].alpha)+delta.y*(epsilon.x*pixels[2].alpha+delta.x*
        pixels[3].alpha));
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
      for (i=0; i < 4L; i++)
      {
        GetPixelInfoPixel(image,p+i*(ssize_t) GetPixelChannels(image),pixels+i);
        AlphaBlendPixelInfo(image,p+i*(ssize_t) GetPixelChannels(image),
          pixels+i,alpha+i);
      }
      gamma=1.0;  /* number of pixels blended together (its variable) */
      for (i=0; i <= 1L; i++)
      {
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
              pixels[i].red+=pixels[i+2].red;
              pixels[i].green+=pixels[i+2].green;
              pixels[i].blue+=pixels[i+2].blue;
              pixels[i].black+=pixels[i+2].black;
              pixels[i].alpha+=pixels[i+2].alpha;
            }
      }
      if ((x-x_offset) >= 0.75)
        {
          alpha[0]=alpha[1];
          pixels[0]=pixels[1];
        }
      else
        if ((x-x_offset) > 0.25)
          {
            gamma*=2.0;  /* blend both rows */
            alpha[0]+= alpha[1];  /* add up alpha weights */
            pixels[0].red+=pixels[1].red;
            pixels[0].green+=pixels[1].green;
            pixels[0].blue+=pixels[1].blue;
            pixels[0].black+=pixels[1].black;
            pixels[0].alpha+=pixels[1].alpha;
          }
      gamma=1.0/gamma;
      alpha[0]=MagickSafeReciprocal(alpha[0]);
      pixel->red=alpha[0]*pixels[0].red;
      pixel->green=alpha[0]*pixels[0].green;  /* divide by sum of alpha */
      pixel->blue=alpha[0]*pixels[0].blue;
      pixel->black=alpha[0]*pixels[0].black;
      pixel->alpha=gamma*pixels[0].alpha;   /* divide by number of pixels */
      break;
    }
    case CatromInterpolatePixel:
    {
      double
        cx[4],
        cy[4];