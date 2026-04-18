// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 33/33



      q=GetAuthenticPixelQueue(image);
      if (image_view != (CacheView *) NULL)
        q=GetCacheViewAuthenticPixelQueue(image_view);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        quantum=GetPixelRed(image,q);
        SetPixelRed(image,GetPixelGreen(image,q),q);
        SetPixelGreen(image,quantum,q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
    }
  if (quantum_info->alpha_type == AssociatedQuantumAlpha)
    {
      double
        gamma,
        Sa;

      /*
        Disassociate alpha.
      */
      q=GetAuthenticPixelQueue(image);
      if (image_view != (CacheView *) NULL)
        q=GetCacheViewAuthenticPixelQueue(image_view);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        ssize_t
          i;

        Sa=QuantumScale*(double) GetPixelAlpha(image,q);
        gamma=MagickSafeReciprocal(Sa);
        for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
        {
          PixelChannel channel = GetPixelChannelChannel(image,i);
          PixelTrait traits = GetPixelChannelTraits(image,channel);
          if ((channel == AlphaPixelChannel) ||
              ((traits & UpdatePixelTrait) == 0))
            continue;
          q[i]=ClampToQuantum(gamma*(double) q[i]);
        }
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
    }
  return(extent);
}
