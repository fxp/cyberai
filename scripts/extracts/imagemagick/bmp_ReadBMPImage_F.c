/* ===== EXTRACT: bmp.c ===== */
/* Function: ReadBMPImage (part F) */
/* Lines: 1179–1313 */

                if (*(p+3) != 0)
                  {
                    image->alpha_trait=BlendPixelTrait;
                    y=-1;
                    break;
                  }
                p+=(ptrdiff_t) 4;
              }
            }
          }
        bmp_info.alpha_mask=image->alpha_trait != UndefinedPixelTrait ?
          0xff000000U : 0U;
        bmp_info.red_mask=0x00ff0000U;
        bmp_info.green_mask=0x0000ff00U;
        bmp_info.blue_mask=0x000000ffU;
        if (bmp_info.bits_per_pixel == 16)
          {
            /*
              RGB555.
            */
            bmp_info.red_mask=0x00007c00U;
            bmp_info.green_mask=0x000003e0U;
            bmp_info.blue_mask=0x0000001fU;
          }
      }
    (void) memset(&shift,0,sizeof(shift));
    (void) memset(&quantum_bits,0,sizeof(quantum_bits));
    if ((bmp_info.bits_per_pixel == 16) || (bmp_info.bits_per_pixel == 32))
      {
        unsigned int
          sample;

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
