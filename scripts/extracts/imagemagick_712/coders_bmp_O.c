// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 15/24



          p=pixels+((ssize_t) image->rows-y-1)*(ssize_t) bytes_per_line;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (Quantum *) NULL)
            break;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            pixel=(unsigned int) (*p++);
            pixel|=((unsigned int) *p++ << 8);
            pixel|=((unsigned int) *p++ << 16);
            pixel|=((unsigned int) *p++ << 24);
            red=((pixel & bmp_info.red_mask) << shift.red) >> 16;
            if (quantum_bits.red == 8)
              red|=(red >> 8);
            green=((pixel & bmp_info.green_mask) << shift.green) >> 16;
            if (quantum_bits.green == 8)
              green|=(green >> 8);
            blue=((pixel & bmp_info.blue_mask) << shift.blue) >> 16;
            if (quantum_bits.blue == 8)
              blue|=(blue >> 8);
            SetPixelRed(image,ScaleShortToQuantum((unsigned short) red),q);
            SetPixelGreen(image,ScaleShortToQuantum((unsigned short) green),q);
            SetPixelBlue(image,ScaleShortToQuantum((unsigned short) blue),q);
            SetPixelAlpha(image,OpaqueAlpha,q);
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