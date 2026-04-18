// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 12/18



        /*
          Set image profiles.
        */
        ResetLinkedListIterator(profiles);
        profile=(StringInfo *) GetNextValueInLinkedList(profiles);
        while (profile != (StringInfo *) NULL)
        {
          (void) SetImageProfilePrivate(image,profile,exception);
          profile=(StringInfo *) GetNextValueInLinkedList(profiles);
        }
        profiles=DestroyLinkedList(profiles,(void *(*)(void *)) NULL);
      }
    duration+=image->delay*image->iterations;
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    opacity=(-1);
    status=SetImageProgress(image,LoadImageTag,(MagickOffsetType)
      image->scene-1,image->scene);
    if (status == MagickFalse)
      break;
  }
  image->duration=duration;
  if (profiles != (LinkedListInfo *) NULL)
    profiles=DestroyLinkedList(profiles,DestroyGIFProfile);
  meta_image=DestroyImage(meta_image);
  global_colormap=(unsigned char *) RelinquishMagickMemory(global_colormap);
  if ((image->columns == 0) || (image->rows == 0))
    ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize");
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r G I F I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterGIFImage() adds properties for the GIF image format to
%  the list of supported formats.  The properties include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterGIFImage method is:
%
%      size_t RegisterGIFImage(void)
%
*/
ModuleExport size_t RegisterGIFImage(void)
{
  MagickInfo
    *entry;