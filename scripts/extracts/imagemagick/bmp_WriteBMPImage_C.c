/* ===== EXTRACT: bmp.c ===== */
/* Function: WriteBMPImage (part C) */
/* Lines: 2103–2249 */

    if (pixel_info == (MemoryInfo *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
    (void) memset(pixels,0,(size_t) bmp_info.image_size);
    switch (bmp_info.bits_per_pixel)
    {
      case 1:
      {
        size_t
          bit,
          byte;

        /*
          Convert PseudoClass image to a BMP monochrome image.
        */
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          ssize_t
            offset;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          q=pixels+((ssize_t) image->rows-y-1)*(ssize_t) bytes_per_line;
          bit=0;
          byte=0;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            byte<<=1;
            byte|=(size_t) (GetPixelIndex(image,p) != 0 ? 0x01 : 0x00);
            bit++;
            if (bit == 8)
              {
                *q++=(unsigned char) byte;
                bit=0;
                byte=0;
              }
             p+=(ptrdiff_t) GetPixelChannels(image);
           }
           if (bit != 0)
             {
               *q++=(unsigned char) (byte << (8-bit));
               x++;
             }
          offset=(ssize_t) (image->columns+7)/8;
          for (x=offset; x < (ssize_t) bytes_per_line; x++)
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
      case 4:
      {
        unsigned int
          byte,
          nibble;

        ssize_t
          offset;

        /*
          Convert PseudoClass image to a BMP monochrome image.
        */
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          q=pixels+((ssize_t) image->rows-y-1)*(ssize_t) bytes_per_line;
          nibble=0;
          byte=0;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            byte<<=4;
            byte|=((unsigned int) GetPixelIndex(image,p) & 0x0f);
            nibble++;
            if (nibble == 2)
              {
                *q++=(unsigned char) byte;
                nibble=0;
                byte=0;
              }
            p+=(ptrdiff_t) GetPixelChannels(image);
          }
          if (nibble != 0)
            {
              *q++=(unsigned char) (byte << 4);
              x++;
            }
          offset=(ssize_t) (image->columns+1)/2;
          for (x=offset; x < (ssize_t) bytes_per_line; x++)
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
      case 8:
      {
        /*
          Convert PseudoClass packet to BMP pixel.
        */
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          q=pixels+((ssize_t) image->rows-y-1)*(ssize_t) bytes_per_line;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            *q++=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
            p+=(ptrdiff_t) GetPixelChannels(image);
          }
          for ( ; x < (ssize_t) bytes_per_line; x++)
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
      case 16:
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
