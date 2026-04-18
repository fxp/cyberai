// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 57/59



    channel=GetPixelChannelChannel(image,i);
    switch (channel)
    {
      case RedPixelChannel:
      {
        name="red";
        if (image->colorspace == CMYKColorspace)
          name="cyan";
        if ((image->colorspace == LinearGRAYColorspace) ||
            (image->colorspace == GRAYColorspace))
          name="gray";
        break;
      }
      case GreenPixelChannel:
      {
        name="green";
        if (image->colorspace == CMYKColorspace)
          name="magenta";
        break;
      }
      case BluePixelChannel:
      {
        name="blue";
        if (image->colorspace == CMYKColorspace)
          name="yellow";
        break;
      }
      case BlackPixelChannel:
      {
        name="black";
        if (image->storage_class == PseudoClass)
          name="index";
        break;
      }
      case IndexPixelChannel:
      {
        name="index";
        break;
      }
      case AlphaPixelChannel:
      {
        name="alpha";
        break;
      }
      case ReadMaskPixelChannel:
      {
        name="read-mask";
        break;
      }
      case WriteMaskPixelChannel:
      {
        name="write-mask";
        break;
      }
      case CompositeMaskPixelChannel:
      {
        name="composite-mask";
        break;
      }
      case MetaPixelChannels:
      {
        name="meta";
        break;
      }
      default:
        name="undefined";
    }
    if (image->colorspace ==  UndefinedColorspace)
      {
        (void) FormatLocaleString(channel_name,MagickPathExtent,"%.20g",
          (double) channel);
        name=(const char *) channel_name;
      }
    *traits='\0';
    if ((GetPixelChannelTraits(image,channel) & UpdatePixelTrait) != 0)
      (void) ConcatenateMagickString(traits,"update,",MagickPathExtent);
    if ((GetPixelChannelTraits(image,channel) & BlendPixelTrait) != 0)
      (void) ConcatenateMagickString(traits,"blend,",MagickPathExtent);
    if ((GetPixelChannelTraits(image,channel) & CopyPixelTrait) != 0)
      (void) ConcatenateMagickString(traits,"copy,",MagickPathExtent);
    if (*traits == '\0')
      (void) ConcatenateMagickString(traits,"undefined,",MagickPathExtent);
    traits[strlen(traits)-1]='\0';
    (void) LogMagickEvent(PixelEvent,GetMagickModule(),"  %.20g: %s (%s)",
      (double) i,name,traits);
  }
}

MagickExport ChannelType SetPixelChannelMask(Image *image,
  const ChannelType channel_mask)
{
#define GetChannelBit(mask,bit)  (((size_t) (mask) >> (size_t) (bit)) & 0x01)

  ChannelType
    mask;

  ssize_t
    i;