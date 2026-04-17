/* ===== EXTRACT: bmp.c ===== */
/* Function: ReadBMPImage (part G) */
/* Lines: 1314–1438 */

              if (status == MagickFalse)
                break;
            }
        }
        (void) SyncImage(image,exception);
        break;
      }
      case 4:
      {
        /*
          Convert PseudoColor scanline.
        */
        for (y=(ssize_t) image->rows-1; y >= 0; y--)
        {
          p=pixels+((ssize_t) image->rows-y-1)*(ssize_t) bytes_per_line;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (Quantum *) NULL)
            break;
          for (x=0; x < ((ssize_t) image->columns-1); x+=2)
          {
            ValidateColormapValue(image,(ssize_t) ((*p >> 4) & 0x0f),&index,
              exception);
            SetPixelIndex(image,index,q);
            q+=(ptrdiff_t) GetPixelChannels(image);
            ValidateColormapValue(image,(ssize_t) (*p & 0x0f),&index,exception);
            SetPixelIndex(image,index,q);
            q+=(ptrdiff_t) GetPixelChannels(image);
            p++;
          }
          if ((image->columns % 2) != 0)
            {
              ValidateColormapValue(image,(ssize_t) ((*p >> 4) & 0xf),&index,
                exception);
              SetPixelIndex(image,index,q);
              q+=(ptrdiff_t) GetPixelChannels(image);
              p++;
              x++;
            }
          if (x < (ssize_t) image->columns)
            break;
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,((MagickOffsetType)
                image->rows-y),image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        (void) SyncImage(image,exception);
        break;
      }
      case 8:
      {
        /*
          Convert PseudoColor scanline.
        */
        if ((bmp_info.compression == BI_RLE8) ||
            (bmp_info.compression == BI_RLE4))
          bytes_per_line=image->columns;
        for (y=(ssize_t) image->rows-1; y >= 0; y--)
        {
          p=pixels+((ssize_t) image->rows-y-1)*(ssize_t) bytes_per_line;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (Quantum *) NULL)
            break;
          for (x=(ssize_t) image->columns; x != 0; --x)
          {
            ValidateColormapValue(image,(ssize_t) *p++,&index,exception);
            SetPixelIndex(image,index,q);
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
        (void) SyncImage(image,exception);
        break;
      }
      case 16:
      {
        unsigned int
          alpha,
          pixel;

        /*
          Convert bitfield encoded 16-bit PseudoColor scanline.
        */
        if ((bmp_info.compression != BI_RGB) &&
            (bmp_info.compression != BI_BITFIELDS))
          {
            pixel_info=RelinquishVirtualMemory(pixel_info);
            ThrowReaderException(CorruptImageError,
              "UnrecognizedImageCompression");
          }
        bytes_per_line=2*(image->columns+image->columns % 2);
        image->storage_class=DirectClass;
        for (y=(ssize_t) image->rows-1; y >= 0; y--)
        {
          p=pixels+((ssize_t) image->rows-y-1)*(ssize_t) bytes_per_line;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (Quantum *) NULL)
            break;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            pixel=(unsigned int) (*p++);
            pixel|=(unsigned int) (*p++) << 8;
            red=((pixel & bmp_info.red_mask) << shift.red) >> 16;
            if (quantum_bits.red == 5)
              red|=((red & 0xff00) >> 5);
            if (quantum_bits.red <= 8)
              red|=((red & 0xff00) >> 8);
            green=((pixel & bmp_info.green_mask) << shift.green) >> 16;
            if (quantum_bits.green == 5)
              green|=((green & 0xff00) >> 5);
            if (quantum_bits.green == 6)
              green|=((green & 0xff00) >> 6);
