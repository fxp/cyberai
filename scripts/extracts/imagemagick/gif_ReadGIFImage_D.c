/* ===== EXTRACT: gif.c ===== */
/* Function: ReadGIFImage (part D) */
/* Lines: 1340–1401 */

    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    status=SetImageExtent(image,image->columns,image->rows,exception);
    if (status == MagickFalse)
      {
        if (profiles != (LinkedListInfo *) NULL)
          profiles=DestroyLinkedList(profiles,DestroyGIFProfile);
        global_colormap=(unsigned char *) RelinquishMagickMemory(
          global_colormap);
        meta_image=DestroyImage(meta_image);
        return(DestroyImageList(image));
      }
    /*
      Decode image.
    */
    if (image_info->ping != MagickFalse)
      status=PingGIFImage(image,exception);
    else
      status=DecodeImage(image,opacity,exception);
    if ((image_info->ping == MagickFalse) && (status == MagickFalse))
      ThrowGIFException(CorruptImageError,"CorruptImage");
    if (profiles != (LinkedListInfo *) NULL)
      {
        StringInfo
          *profile;

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
