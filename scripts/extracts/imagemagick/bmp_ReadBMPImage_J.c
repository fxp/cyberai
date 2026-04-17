/* ===== EXTRACT: bmp.c ===== */
/* Function: ReadBMPImage (part J) */
/* Lines: 1679–1721 */

                if (image->debug != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "Profile: ICC, %u bytes",(unsigned int) profile_size_orig);
                (void) SetImageProfilePrivate(image,profile,exception);
              }
          }
      }
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    offset=(MagickOffsetType) bmp_info.ba_offset;
    if (offset != 0)
      if ((offset < TellBlob(image)) ||
          (SeekBlob(image,offset,SEEK_SET) != offset))
        ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    *magick='\0';
    count=ReadBlob(image,2,magick);
    if ((count == 2) && (IsBMP(magick,2) != MagickFalse))
      {
        /*
          Acquire next image structure.
        */
        AcquireNextImage(image_info,image,exception);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            status=MagickFalse;
            break;
          }
        image=SyncNextImageInList(image);
        status=SetImageProgress(image,LoadImagesTag,TellBlob(image),blob_size);
        if (status == MagickFalse)
          break;
      }
  } while (IsBMP(magick,2) != MagickFalse);
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}
