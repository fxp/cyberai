// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 56/59



  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  (void) memset(image->channel_map,0,MaxPixelChannels*
    sizeof(*image->channel_map));
  trait=UpdatePixelTrait;
  if (image->alpha_trait != UndefinedPixelTrait)
    trait=(PixelTrait) (trait | BlendPixelTrait);
  n=0;
  if ((image->colorspace == LinearGRAYColorspace) ||
      (image->colorspace == GRAYColorspace))
    {
      SetPixelChannelAttributes(image,BluePixelChannel,trait,n);
      SetPixelChannelAttributes(image,GreenPixelChannel,trait,n);
      SetPixelChannelAttributes(image,RedPixelChannel,trait,n++);
    }
  else
    {
      SetPixelChannelAttributes(image,RedPixelChannel,trait,n++);
      SetPixelChannelAttributes(image,GreenPixelChannel,trait,n++);
      SetPixelChannelAttributes(image,BluePixelChannel,trait,n++);
    }
  if (image->colorspace == CMYKColorspace)
    SetPixelChannelAttributes(image,BlackPixelChannel,trait,n++);
  if (image->alpha_trait != UndefinedPixelTrait)
    SetPixelChannelAttributes(image,AlphaPixelChannel,CopyPixelTrait,n++);
  if (image->storage_class == PseudoClass)
    SetPixelChannelAttributes(image,IndexPixelChannel,CopyPixelTrait,n++);
  if ((image->channels & ReadMaskChannel) != 0)
    SetPixelChannelAttributes(image,ReadMaskPixelChannel,CopyPixelTrait,n++);
  if ((image->channels & WriteMaskChannel) != 0)
    SetPixelChannelAttributes(image,WriteMaskPixelChannel,CopyPixelTrait,n++);
  if ((image->channels & CompositeMaskChannel) != 0)
    SetPixelChannelAttributes(image,CompositeMaskPixelChannel,CopyPixelTrait,
      n++);
  if (image->number_meta_channels != 0)
    {
      PixelChannel
        meta_channel;

      ssize_t
        i;

      if (image->number_meta_channels >= (size_t) (MaxPixelChannels-MetaPixelChannels))
        {
          image->number_channels=(size_t) n;
          image->number_meta_channels=0;
          (void) SetPixelChannelMask(image,image->channel_mask);
          ThrowBinaryException(CorruptImageError,"MaximumChannelsExceeded",
            image->filename);
        }
      meta_channel=MetaPixelChannels;
      for (i=0; i < (ssize_t) image->number_meta_channels; i++)
      {
        SetPixelChannelAttributes(image,meta_channel,UpdatePixelTrait,n);
        meta_channel=(PixelChannel) (meta_channel+1);
        n++;
      }
    }
  image->number_channels=(size_t) n;
  (void) SetPixelChannelMask(image,image->channel_mask);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t P i x e l C h a n n e l M a s k                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetPixelChannelMask() sets the pixel channel map from the specified channel
%  mask.
%
%  The format of the SetPixelChannelMask method is:
%
%      ChannelType SetPixelChannelMask(Image *image,
%        const ChannelType channel_mask)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o channel_mask: the channel mask.
%
*/

static void LogPixelChannels(const Image *image)
{
  ssize_t
    i;

  (void) LogMagickEvent(PixelEvent,GetMagickModule(),"%s[0x%08llx]",
    image->filename,(MagickOffsetType) image->channel_mask);
  for (i=0; i < (ssize_t) image->number_channels; i++)
  {
    char
      channel_name[MagickPathExtent],
      traits[MagickPathExtent];

    const char
      *name;

    PixelChannel
      channel;