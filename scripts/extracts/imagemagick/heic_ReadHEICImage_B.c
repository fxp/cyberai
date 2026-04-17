/* ===== EXTRACT: heic.c ===== */
/* Function: ReadHEICImage (part B) */
/* Lines: 748–766 */

          if (image->scene >= (image_info->scene+image_info->number_scenes-1))
            break;
      }
      ids=(heif_item_id *) RelinquishMagickMemory(ids);
    }
  error=heif_context_get_image_handle(heif_context,primary_image_id,
    &image_handle);
  if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
    {
      heif_context_free(heif_context);
      return(DestroyImageList(image));
    }
  ReadHEICDepthImage(image_info,image,image_handle,exception);
  heif_image_handle_release(image_handle);
  heif_context_free(heif_context);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}
