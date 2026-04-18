// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 58/59



  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(PixelEvent,GetMagickModule(),"%s[0x%08llx]",
      image->filename,(MagickOffsetType) channel_mask);
  mask=image->channel_mask;
  image->channel_mask=channel_mask;
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    if (GetChannelBit(channel_mask,channel) == 0)
      {
        SetPixelChannelTraits(image,channel,CopyPixelTrait);
        continue;
      }
    if (channel == AlphaPixelChannel)
      {
        if ((image->alpha_trait & CopyPixelTrait) != 0)
          {
            SetPixelChannelTraits(image,channel,CopyPixelTrait);
            continue;
          }
        SetPixelChannelTraits(image,channel,UpdatePixelTrait);
        continue;
      }
    if (image->alpha_trait != UndefinedPixelTrait)
      {
        SetPixelChannelTraits(image,channel,(PixelTrait) (UpdatePixelTrait |
          BlendPixelTrait));
        continue;
      }
    SetPixelChannelTraits(image,channel,UpdatePixelTrait);
  }
  if (image->storage_class == PseudoClass)
    SetPixelChannelTraits(image,IndexPixelChannel,CopyPixelTrait);
  if ((image->channels & ReadMaskChannel) != 0)
    SetPixelChannelTraits(image,ReadMaskPixelChannel,CopyPixelTrait);
  if ((image->channels & WriteMaskChannel) != 0)
    SetPixelChannelTraits(image,WriteMaskPixelChannel,CopyPixelTrait);
  if ((image->channels & CompositeMaskChannel) != 0)
    SetPixelChannelTraits(image,CompositeMaskPixelChannel,CopyPixelTrait);
  if ((GetLogEventMask() & PixelEvent) != 0)
    LogPixelChannels(image);
  return(mask);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t P i x e l M e t a C h a n n e l s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetPixelMetaChannels() sets the image meta channels.
%
%  The format of the SetPixelMetaChannels method is:
%
%      MagickBooleanType SetPixelMetaChannels(Image *image,
%        const size_t number_meta_channels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o number_meta_channels:  the number of meta channels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetPixelMetaChannels(Image *image,
  const size_t number_meta_channels,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  if (number_meta_channels >= (size_t) (MaxPixelChannels-MetaPixelChannels))
   ThrowBinaryException(CorruptImageError,"MaximumChannelsExceeded",
     image->filename);
  image->number_meta_channels=number_meta_channels;
  status=ResetPixelChannelMap(image,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  return(SyncImagePixelCache(image,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S o r t I m a g e P i x e l s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SortImagePixels() sorts pixels within each scanline in ascending order of
%  intensity.
%
%  The format of the SortImagePixels method is:
%
%      MagickBooleanType SortImagePixels(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SortImagePixels(Image *image,
  ExceptionInfo *exception)
{
#define SolarizeImageTag  "Solarize/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;