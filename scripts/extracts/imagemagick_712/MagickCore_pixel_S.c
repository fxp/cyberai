// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 51/59



      p=GetCacheViewVirtualPixels(image_view,x_offset-1,y_offset-1,4,4,
        exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      for (i=0; i < 16L; i++)
        AlphaBlendPixelInfo(image,p+i*(ssize_t) GetPixelChannels(image),
          pixels+i,alpha+i);
      CatromWeights((double) (x-x_offset),&cx);
      CatromWeights((double) (y-y_offset),&cy);
      pixel->red=(cy[0]*(cx[0]*pixels[0].red+cx[1]*pixels[1].red+cx[2]*
        pixels[2].red+cx[3]*pixels[3].red)+cy[1]*(cx[0]*pixels[4].red+cx[1]*
        pixels[5].red+cx[2]*pixels[6].red+cx[3]*pixels[7].red)+cy[2]*(cx[0]*
        pixels[8].red+cx[1]*pixels[9].red+cx[2]*pixels[10].red+cx[3]*
        pixels[11].red)+cy[3]*(cx[0]*pixels[12].red+cx[1]*pixels[13].red+cx[2]*
        pixels[14].red+cx[3]*pixels[15].red));
      pixel->green=(cy[0]*(cx[0]*pixels[0].green+cx[1]*pixels[1].green+cx[2]*
        pixels[2].green+cx[3]*pixels[3].green)+cy[1]*(cx[0]*pixels[4].green+
        cx[1]*pixels[5].green+cx[2]*pixels[6].green+cx[3]*pixels[7].green)+
        cy[2]*(cx[0]*pixels[8].green+cx[1]*pixels[9].green+cx[2]*
        pixels[10].green+cx[3]*pixels[11].green)+cy[3]*(cx[0]*
        pixels[12].green+cx[1]*pixels[13].green+cx[2]*pixels[14].green+cx[3]*
        pixels[15].green));
      pixel->blue=(cy[0]*(cx[0]*pixels[0].blue+cx[1]*pixels[1].blue+cx[2]*
        pixels[2].blue+cx[3]*pixels[3].blue)+cy[1]*(cx[0]*pixels[4].blue+cx[1]*
        pixels[5].blue+cx[2]*pixels[6].blue+cx[3]*pixels[7].blue)+cy[2]*(cx[0]*
        pixels[8].blue+cx[1]*pixels[9].blue+cx[2]*pixels[10].blue+cx[3]*
        pixels[11].blue)+cy[3]*(cx[0]*pixels[12].blue+cx[1]*pixels[13].blue+
        cx[2]*pixels[14].blue+cx[3]*pixels[15].blue));
      if (image->colorspace == CMYKColorspace)
        pixel->black=(cy[0]*(cx[0]*pixels[0].black+cx[1]*pixels[1].black+cx[2]*
          pixels[2].black+cx[3]*pixels[3].black)+cy[1]*(cx[0]*pixels[4].black+
          cx[1]*pixels[5].black+cx[2]*pixels[6].black+cx[3]*pixels[7].black)+
          cy[2]*(cx[0]*pixels[8].black+cx[1]*pixels[9].black+cx[2]*
          pixels[10].black+cx[3]*pixels[11].black)+cy[3]*(cx[0]*
          pixels[12].black+cx[1]*pixels[13].black+cx[2]*pixels[14].black+cx[3]*
          pixels[15].black));
      pixel->alpha=(cy[0]*(cx[0]*pixels[0].alpha+cx[1]*pixels[1].alpha+cx[2]*
        pixels[2].alpha+cx[3]*pixels[3].alpha)+cy[1]*(cx[0]*pixels[4].alpha+
        cx[1]*pixels[5].alpha+cx[2]*pixels[6].alpha+cx[3]*pixels[7].alpha)+
        cy[2]*(cx[0]*pixels[8].alpha+cx[1]*pixels[9].alpha+cx[2]*
        pixels[10].alpha+cx[3]*pixels[11].alpha)+cy[3]*(cx[0]*pixels[12].alpha+
        cx[1]*pixels[13].alpha+cx[2]*pixels[14].alpha+cx[3]*pixels[15].alpha));
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
      GetPixelInfoPixel(image,p,pixel);
      break;
    }
    case MeshInterpolatePixel:
    {
      PointInfo
        delta,
        luminance;