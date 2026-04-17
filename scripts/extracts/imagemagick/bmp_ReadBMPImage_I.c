/* ===== EXTRACT: bmp.c ===== */
/* Function: ReadBMPImage (part I) */
/* Lines: 1548–1678 */

            if (image->alpha_trait != UndefinedPixelTrait)
              {
                alpha=((pixel & bmp_info.alpha_mask) << shift.alpha) >> 16;
                if (quantum_bits.alpha == 8)
                  alpha|=(alpha >> 8);
                SetPixelAlpha(image,ScaleShortToQuantum(
                  (unsigned short) alpha),q);
              }
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          offset=((MagickOffsetType) image->rows-y-1);
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,((MagickOffsetType)
                image->rows-y),image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case 64:
      {
        /*
          Convert DirectColor scanline.
        */
        bytes_per_line=4*((image->columns*64+31)/32);
        for (y=(ssize_t) image->rows-1; y >= 0; y--)
        {
          unsigned short
            *p16;

          p16=(unsigned short *) (pixels+((ssize_t) image->rows-y-1)*
            (ssize_t) bytes_per_line);
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (Quantum *) NULL)
            break;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            SetPixelBlue(image,ScaleShortToQuantum(*p16++),q);
            SetPixelGreen(image,ScaleShortToQuantum(*p16++),q);
            SetPixelRed(image,ScaleShortToQuantum(*p16++),q);
            SetPixelAlpha(image,ScaleShortToQuantum(*p16++),q);
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          offset=((MagickOffsetType) image->rows-y-1);
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,((MagickOffsetType)
                image->rows-y),image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      default:
      {
        pixel_info=RelinquishVirtualMemory(pixel_info);
        ThrowReaderException(CorruptImageError,"ImproperImageHeader");
      }
    }
    pixel_info=RelinquishVirtualMemory(pixel_info);
    if (y > 0)
      break;
    if (EOFBlob(image) != MagickFalse)
      {
        ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
        break;
      }
    if (bmp_info.height < 0)
      {
        Image
          *flipped_image;

        /*
          Correct image orientation.
        */
        flipped_image=FlipImage(image,exception);
        if (flipped_image != (Image *) NULL)
          {
            DuplicateBlob(flipped_image,image);
            ReplaceImageInList(&image, flipped_image);
            image=flipped_image;
          }
      }
    /*
      Read embedded ICC profile
    */
    if ((bmp_info.colorspace == 0x4D424544L) && (profile_data > 0) &&
        (profile_size > 0))
      {
        StringInfo
          *profile;

        unsigned char
          *datum;

        offset=start_position+14+profile_data;
        if ((offset < TellBlob(image)) ||
            (SeekBlob(image,offset,SEEK_SET) != offset) ||
            (blob_size < (MagickSizeType) (offset+profile_size)))
          ThrowReaderException(CorruptImageError,"ImproperImageHeader");
        profile=AcquireProfileStringInfo("icc",(size_t) profile_size,
          exception);
        if (profile == (StringInfo *) NULL)
          (void) SeekBlob(image,profile_size,SEEK_CUR);
        else
          {
            datum=GetStringInfoDatum(profile);
            if (ReadBlob(image,(size_t) profile_size,datum) != (ssize_t) profile_size)
              profile=DestroyStringInfo(profile);
            else
              {
                MagickOffsetType
                  profile_size_orig;

                /*
                 Trimming padded bytes.
                */
                profile_size_orig=(MagickOffsetType) datum[0] << 24;
                profile_size_orig|=(MagickOffsetType) datum[1] << 16;
                profile_size_orig|=(MagickOffsetType) datum[2] << 8;
                profile_size_orig|=(MagickOffsetType) datum[3];
                if (profile_size_orig < profile_size)
                  SetStringInfoLength(profile,(size_t) profile_size_orig);
