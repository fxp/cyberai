/* ===== EXTRACT: psd.c ===== */
/* Function: ReadPSDImage (part C) */
/* Lines: 2631–2763 */

          if (profile != (StringInfo *) NULL)
            profile=DestroyStringInfo(profile);
          (void) CloseBlob(image);
          image=DestroyImageList(image);
          return((Image *) NULL);
        }

      /*
         Skip the rest of the layer and mask information.
      */
      (void) SeekBlob(image,offset+(MagickOffsetType) length,SEEK_SET);
    }
  /*
    If we are only "pinging" the image, then we're done - so return.
  */
  if (EOFBlob(image) != MagickFalse)
    {
      if (profile != (StringInfo *) NULL)
        profile=DestroyStringInfo(profile);
      ThrowReaderException(CorruptImageError,"UnexpectedEndOfFile");
    }
  if (image_info->ping != MagickFalse)
    {
      if (profile != (StringInfo *) NULL)
        profile=DestroyStringInfo(profile);
      (void) CloseBlob(image);
      return(GetFirstImageInList(image));
    }
  /*
    Read the precombined layer, present for PSD < 4 compatibility.
  */
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  reading the precombined layer");
  image_list_length=GetImageListLength(image);
  if ((psd_info.has_merged_image != MagickFalse) || (image_list_length == 1))
    psd_info.has_merged_image=(MagickBooleanType) ReadPSDMergedImage(
      image_info,image,&psd_info,exception);
  if ((psd_info.has_merged_image == MagickFalse) && (image_list_length == 1) &&
      (length != 0))
    {
      (void) SeekBlob(image,offset,SEEK_SET);
      status=ReadPSDLayersInternal(image,image_info,&psd_info,MagickFalse,
        exception);
      if (status != MagickTrue)
        {
          if (profile != (StringInfo *) NULL)
            profile=DestroyStringInfo(profile);
          (void) CloseBlob(image);
          image=DestroyImageList(image);
          return((Image *) NULL);
        }
      image_list_length=GetImageListLength(image);
    }
  if (psd_info.has_merged_image == MagickFalse)
    {
      Image
        *merged;

      if (image_list_length == 1)
        {
          if (profile != (StringInfo *) NULL)
            profile=DestroyStringInfo(profile);
          ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
        }
      image->background_color.alpha=(MagickRealType) TransparentAlpha;
      image->background_color.alpha_trait=BlendPixelTrait;
      (void) SetImageBackgroundColor(image,exception);
      merged=MergeImageLayers(image,FlattenLayer,exception);
      if (merged == (Image *) NULL)
        {
          if (profile != (StringInfo *) NULL)
            profile=DestroyStringInfo(profile);
          (void) CloseBlob(image);
          image=DestroyImageList(image);
          return((Image *) NULL);
        }
      ReplaceImageInList(&image,merged);
    }
  if (profile != (StringInfo *) NULL)
    {
      const char
        *option;

      Image
        *next;

      MagickBooleanType
        replicate_profile;

      option=GetImageOption(image_info,"psd:replicate-profile");
      replicate_profile=IsStringTrue(option);
      i=0;
      next=image;
      while (next != (Image *) NULL)
      {
        if (PSDSkipImage(&psd_info,image_info,(size_t) i++) == MagickFalse)
          {
            (void) SetImageProfile(next,GetStringInfoName(profile),profile,
              exception);
            if (replicate_profile == MagickFalse)
              break;
          }
        next=next->next;
      }
      profile=DestroyStringInfo(profile);
      /*
        Always replicate the color profile to all images.
      */
      if ((replicate_profile == MagickFalse) &&
          (image->next != (Image *) NULL))
        {
          const StringInfo
            *icc_profile;

          icc_profile=GetImageProfile(image,"icc");
          if (icc_profile != (const StringInfo *) NULL)
            {
              next=image->next;
              while (next != (Image *) NULL)
              {          
                (void) SetImageProfile(next,"icc",icc_profile,exception);
                next=next->next;
              }
            }
        }
    }
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}
