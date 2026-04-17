/* ===== EXTRACT: tiff.c ===== */
/* Function: ReadTIFFImage (part I) */
/* Lines: 2149–2231 */

            for (x=0; x < (ssize_t) image->columns; x++)
            {
              SetPixelRed(image,ScaleCharToQuantum((unsigned char)
                TIFFGetR(*p)),q);
              SetPixelGreen(image,ScaleCharToQuantum((unsigned char)
                TIFFGetG(*p)),q);
              SetPixelBlue(image,ScaleCharToQuantum((unsigned char)
                TIFFGetB(*p)),q);
              if (image->alpha_trait != UndefinedPixelTrait)
                SetPixelAlpha(image,ScaleCharToQuantum((unsigned char)
                  TIFFGetA(*p)),q);
              p--;
              q-=GetPixelChannels(image);
            }
            if (SyncAuthenticPixels(image,exception) == MagickFalse)
              break;
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
                  image->rows);
                if (status == MagickFalse)
                  break;
              }
          }
          generic_info=RelinquishVirtualMemory(generic_info);
          break;
        }
      }
    pixel_info=RelinquishVirtualMemory(pixel_info);
    SetQuantumImageType(image,quantum_type);
  next_tiff_frame:
    if (quantum_info != (QuantumInfo *) NULL)
      quantum_info=DestroyQuantumInfo(quantum_info);
    if (tiff_status == -1)
      {
        status=MagickFalse;
        break;
      }
    if (photometric == PHOTOMETRIC_CIELAB)
      DecodeLabImage(image,exception);
    if ((photometric == PHOTOMETRIC_LOGL) ||
        (photometric == PHOTOMETRIC_MINISBLACK) ||
        (photometric == PHOTOMETRIC_MINISWHITE))
      {
        image->type=GrayscaleType;
        if (bits_per_sample == 1)
          image->type=BilevelType;
      }
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    more_frames=TIFFReadDirectory(tiff) != 0 ? MagickTrue : MagickFalse;
    if (more_frames != MagickFalse)
      {
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
        status=SetImageProgress(image,LoadImagesTag,(MagickOffsetType)
          image->scene-1,image->scene);
        if (status == MagickFalse)
          break;
      }
  } while ((status != MagickFalse) && (more_frames != MagickFalse));
  TIFFClose(tiff);
  if (status != MagickFalse)
    TIFFReadPhotoshopLayers(image_info,image,exception);
  if ((image_info->number_scenes != 0) &&
      (image_info->scene >= GetImageListLength(image)))
    status=MagickFalse;
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}
