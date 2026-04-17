/* ===== EXTRACT: bmp.c ===== */
/* Function: WriteBMPImage (part D) */
/* Lines: 2250–2364 */

          for (x=0; x < (ssize_t) image->columns; x++)
          {
            unsigned short
              pixel;

            pixel=0;
            if (bmp_subtype == ARGB4444)
              {
                pixel=(unsigned short) (ScaleQuantumToAny(
                  GetPixelAlpha(image,p),15) << 12);
                pixel|=(unsigned short) (ScaleQuantumToAny(
                  GetPixelRed(image,p),15) << 8);
                pixel|=(unsigned short) (ScaleQuantumToAny(
                  GetPixelGreen(image,p),15) << 4);
                pixel|=(unsigned short) (ScaleQuantumToAny(
                  GetPixelBlue(image,p),15));
              }
            else if (bmp_subtype == RGB565)
              {
                pixel=(unsigned short) (ScaleQuantumToAny(
                  GetPixelRed(image,p),31) << 11);
                pixel|=(unsigned short) (ScaleQuantumToAny(
                  GetPixelGreen(image,p),63) << 5);
                pixel|=(unsigned short) (ScaleQuantumToAny(
                  GetPixelBlue(image,p),31));
              }
            else
              {
                if (bmp_subtype == ARGB1555)
                  pixel=(unsigned short) (ScaleQuantumToAny(
                    GetPixelAlpha(image,p),1) << 15);
                pixel|=(unsigned short) (ScaleQuantumToAny(
                  GetPixelRed(image,p),31) << 10);
                pixel|=(unsigned short) (ScaleQuantumToAny(
                  GetPixelGreen(image,p),31) << 5);
                pixel|=(unsigned short) (ScaleQuantumToAny(
                  GetPixelBlue(image,p),31));
              }
            *((unsigned short *) q)=pixel;
            q+=(ptrdiff_t) 2;
            p+=(ptrdiff_t) GetPixelChannels(image);
          }
          for (x=2L*(ssize_t) image->columns; x < (ssize_t) bytes_per_line; x++)
            *q++=0x00;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case 24:
      {
        /*
          Convert DirectClass packet to BMP BGR888.
        */
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          q=pixels+((ssize_t) image->rows-y-1)*(ssize_t) bytes_per_line;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
            *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
            *q++=ScaleQuantumToChar(GetPixelRed(image,p));
            p+=(ptrdiff_t) GetPixelChannels(image);
          }
          for (x=3L*(ssize_t) image->columns; x < (ssize_t) bytes_per_line; x++)
            *q++=0x00;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case 32:
      {
        /*
          Convert DirectClass packet to ARGB8888 pixel.
        */
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          q=pixels+((ssize_t) image->rows-y-1)*(ssize_t) bytes_per_line;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
            *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
            *q++=ScaleQuantumToChar(GetPixelRed(image,p));
            *q++=ScaleQuantumToChar(GetPixelAlpha(image,p));
            p+=(ptrdiff_t) GetPixelChannels(image);
          }
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
    }
    if ((type > 2) && (bmp_info.bits_per_pixel == 8))
