// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 18/34



            q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
            if (q == (Quantum *) NULL)
              break;
            q+=(ptrdiff_t) GetPixelChannels(image)*(image->columns-1);
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
    SetQuantumImageType(image,image_quantum_type);
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
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r T I F F I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterTIFFImage() adds properties for the TIFF image format to
%  the list of supported formats.  The properties include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterTIFFImage method is:
%
%      size_t RegisterTIFFImage(void)
%
*/

#if defined(MAGICKCORE_TIFF_DELEGATE)
static TIFFExtendProc
  tag_extender = (TIFFExtendProc) NULL;

static void TIFFIgnoreTags(TIFF *tiff)
{
  char
    *q;

  const char
    *p,
    *tags;

  Image
   *image;

  size_t
    count;

  ssize_t
    i;

  static const
    char *dummy_name = "";

  TIFFFieldInfo
    *ignore;

  if (TIFFGetReadProc(tiff) != TIFFReadBlob)
    return;
  image=(Image *)TIFFClientdata(tiff);
  tags=GetImageArtifact(image,"tiff:ignore-tags");
  if (tags == (const char *) NULL)
    return;
  count=0;
  p=tags;
  while (*p != '\0')
  {
    while ((isspace((int) ((unsigned char) *p)) != 0))
      p++;

    (void) strtol(p,&q,10);
    if (p == q)
      return;