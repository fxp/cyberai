// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 48/59



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
            pixels[j]=alpha[j]*(double) p[j*(ssize_t) GetPixelChannels(source)+
              i];
          }
        SplineWeights((double) (x-x_offset),&cx);
        SplineWeights((double) (y-y_offset),&cy);
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
  }
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n t e r p o l a t e P i x e l I n f o                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpolatePixelInfo() applies a pixel interpolation method between a
%  floating point coordinate and the pixels surrounding that coordinate.  No
%  pixel area resampling, or scaling of the result is performed.
%
%  Interpolation is restricted to just RGBKA channels.
%
%  The format of the InterpolatePixelInfo method is:
%
%      MagickBooleanType InterpolatePixelInfo(const Image *image,
%        const CacheView *image_view,const PixelInterpolateMethod method,
%        const double x,const double y,PixelInfo *pixel,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o image_view: the image view.
%
%    o method: the pixel color interpolation method.
%
%    o x,y: A double representing the current (x,y) position of the pixel.
%
%    o pixel: return the interpolated pixel here.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline void AlphaBlendPixelInfo(const Image *image,
  const Quantum *pixel,PixelInfo *pixel_info,double *alpha)
{
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    {
      *alpha=1.0;
      pixel_info->red=(double) GetPixelRed(image,pixel);
      pixel_info->green=(double) GetPixelGreen(image,pixel);
      pixel_info->blue=(double) GetPixelBlue(image,pixel);
      pixel_info->black=0.0;
      if (image->colorspace == CMYKColorspace)
        pixel_info->black=(double) GetPixelBlack(image,pixel);
      pixel_info->alpha=(double) GetPixelAlpha(image,pixel);
      return;
    }
  *alpha=QuantumScale*(double) GetPixelAlpha(image,pixel);
  pixel_info->red=(*alpha*(double) GetPixelRed(image,pixel));
  pixel_info->green=(*alpha*(double) GetPixelGreen(image,pixel));
  pixel_info->blue=(*alpha*(double) GetPixelBlue(image,pixel));
  pixel_info->black=0.0;
  if (image->colorspace == CMYKColorspace)
    pixel_info->black=(*alpha*(double) GetPixelBlack(image,pixel));
  pixel_info->alpha=(double) GetPixelAlpha(image,pixel);
}

MagickExport MagickBooleanType InterpolatePixelInfo(const Image *image,
  const CacheView_ *image_view,const PixelInterpolateMethod method,
  const double x,const double y,PixelInfo *pixel,ExceptionInfo *exception)
{
  const Quantum
    *p;

  double
    alpha[16],
    gamma;

  MagickBooleanType
    status;

  PixelInfo
    pixels[16];

  PixelInterpolateMethod
    interpolate;

  ssize_t
    i,
    x_offset,
    y_offset;