// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 12/24



        /*
          Get shift and quantum bits info from bitfield masks.
        */
        if (bmp_info.red_mask != 0)
          while (((bmp_info.red_mask << shift.red) & 0x80000000UL) == 0)
          {
            shift.red++;
            if (shift.red >= 32U)
              break;
          }
        if (bmp_info.green_mask != 0)
          while (((bmp_info.green_mask << shift.green) & 0x80000000UL) == 0)
          {
            shift.green++;
            if (shift.green >= 32U)
              break;
          }
        if (bmp_info.blue_mask != 0)
          while (((bmp_info.blue_mask << shift.blue) & 0x80000000UL) == 0)
          {
            shift.blue++;
            if (shift.blue >= 32U)
              break;
          }
        if (bmp_info.alpha_mask != 0)
          while (((bmp_info.alpha_mask << shift.alpha) & 0x80000000UL) == 0)
          {
            shift.alpha++;
            if (shift.alpha >= 32U)
              break;
          }
        sample=shift.red;
        while (((bmp_info.red_mask << sample) & 0x80000000UL) != 0)
        {
          sample++;
          if (sample >= 32U)
            break;
        }
        quantum_bits.red=(MagickRealType) (sample-shift.red);
        sample=shift.green;
        while (((bmp_info.green_mask << sample) & 0x80000000UL) != 0)
        {
          sample++;
          if (sample >= 32U)
            break;
        }
        quantum_bits.green=(MagickRealType) (sample-shift.green);
        sample=shift.blue;
        while (((bmp_info.blue_mask << sample) & 0x80000000UL) != 0)
        {
          sample++;
          if (sample >= 32U)
            break;
        }
        quantum_bits.blue=(MagickRealType) (sample-shift.blue);
        sample=shift.alpha;
        while (((bmp_info.alpha_mask << sample) & 0x80000000UL) != 0)
        {
          sample++;
          if (sample >= 32U)
            break;
        }
        quantum_bits.alpha=(MagickRealType) (sample-shift.alpha);
      }
    switch (bmp_info.bits_per_pixel)
    {
      case 1:
      {
        /*
          Convert bitmap scanline.
        */
        for (y=(ssize_t) image->rows-1; y >= 0; y--)
        {
          p=pixels+((ssize_t) image->rows-y-1)*(ssize_t) bytes_per_line;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (Quantum *) NULL)
            break;
          for (x=0; x < ((ssize_t) image->columns-7); x+=8)
          {
            for (bit=0; bit < 8; bit++)
            {
              index=(Quantum) (((*p) & (0x80 >> bit)) != 0 ? 0x01 : 0x00);
              SetPixelIndex(image,index,q);
              q+=(ptrdiff_t) GetPixelChannels(image);
            }
            p++;
          }
          if ((image->columns % 8) != 0)
            {
              for (bit=0; bit < (image->columns % 8); bit++)
              {
                index=(Quantum) (((*p) & (0x80 >> bit)) != 0 ? 0x01 : 0x00);
                SetPixelIndex(image,index,q);
                q+=(ptrdiff_t) GetPixelChannels(image);
              }
              p++;
            }
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
            