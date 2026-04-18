// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 9/17



      ids=(heif_item_id *) AcquireQuantumMemory((size_t) count,sizeof(*ids));
      if (ids == (heif_item_id *) NULL)
        {
          heif_context_free(heif_context);
          return(DestroyImageList(image));
        }
      (void) heif_context_get_list_of_top_level_image_IDs(heif_context,ids,
        (int) count);
      for (i=0; i < count; i++)
      {
        if (ids[i] == primary_image_id)
          continue;
        /*
          Allocate next image structure.
        */
        AcquireNextImage(image_info,image,exception);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            status=MagickFalse;
            break;
          }
        image=SyncNextImageInList(image);
        error=heif_context_get_image_handle(heif_context,ids[i],&image_handle);
        if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
          {
            status=MagickFalse;
            break;
          }
        status=ReadHEICImageHandle(image_info,image,heif_context,image_handle,
          exception);
        heif_image_handle_release(image_handle);
        if (status == MagickFalse)
          break;
        if (image_info->number_scenes != 0)
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
  ReadHEICDepthImage(image_info,image,heif_context,image_handle,exception);
  heif_image_handle_release(image_handle);
  heif_context_free(heif_context);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s H E I C                                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsHEIC() returns MagickTrue if the image format type, identified by the
%  magick string, is Heic.
%
%  The format of the IsHEIC method is:
%
%      MagickBooleanType IsHEIC(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: compare image format pattern against these bytes.
%
%    o length: Specifies the length of the magick string.
%
*/
static MagickBooleanType IsHEIC(const unsigned char *magick,const size_t length)
{
#if defined(MAGICKCORE_HEIC_DELEGATE)
  enum heif_filetype_result
    type;

  if (length < 12)
    return(MagickFalse);
  type=heif_check_filetype(magick,(int) length);
  if (type == heif_filetype_yes_supported)
    return(MagickTrue);
#else
  magick_unreferenced(magick);
  magick_unreferenced(length);
#endif
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r H E I C I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterHEICImage() adds attributes for the HEIC image format to the list of
%  supported formats.  The attributes include the image format tag, a method
%  to read and/or write the format, whether the format supports the saving of
%  more than one frame to the same file or blob, whether the format supports
%  native in-memory I/O, and a brief description of the format.
%
%  The format of the RegisterHEICImage method is:
%
%      size_t RegisterHEICImage(void)
%
*/
ModuleExport size_t RegisterHEICImage(void)
{
  MagickInfo
    *entry;