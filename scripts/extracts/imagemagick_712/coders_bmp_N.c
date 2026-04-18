// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 14/24



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
            if (quantum_bits.green <= 8)
              green|=((green & 0xff00) >> 8);
            blue=((pixel & bmp_info.blue_mask) << shift.blue) >> 16;
            if (quantum_bits.blue == 5)
              blue|=((blue & 0xff00) >> 5);
            if (quantum_bits.blue <= 8)
              blue|=((blue & 0xff00) >> 8);
            SetPixelRed(image,ScaleShortToQuantum((unsigned short) red),q);
            SetPixelGreen(image,ScaleShortToQuantum((unsigned short) green),q);
            SetPixelBlue(image,ScaleShortToQuantum((unsigned short) blue),q);
            SetPixelAlpha(image,OpaqueAlpha,q);
            if (image->alpha_trait != UndefinedPixelTrait)
              {
                alpha=((pixel & bmp_info.alpha_mask) << shift.alpha) >> 16;
                if (quantum_bits.alpha <= 8)
                  alpha|=((alpha & 0xff00) >> 8);
                SetPixelAlpha(image,ScaleShortToQuantum((unsigned short)
                  alpha),q);
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
      case 24:
      {
        /*
          Convert DirectColor scanline.
        */
        bytes_per_line=4*((image->columns*24+31)/32);
        for (y=(ssize_t) image->rows-1; y >= 0; y--)
        {
          p=pixels+((ssize_t) image->rows-y-1)*(ssize_t) bytes_per_line;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (Quantum *) NULL)
            break;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
            SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
            SetPixelRed(image,ScaleCharToQuantum(*p++),q);
            SetPixelAlpha(image,OpaqueAlpha,q);
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
      case 32:
      {
        /*
          Convert bitfield encoded DirectColor scanline.
        */
        if ((bmp_info.compression != BI_RGB) &&
            (bmp_info.compression != BI_BITFIELDS))
          {
            pixel_info=RelinquishVirtualMemory(pixel_info);
            ThrowReaderException(CorruptImageError,
              "UnrecognizedImageCompression");
          }
        bytes_per_line=4*(image->columns);
        for (y=(ssize_t) image->rows-1; y >= 0; y--)
        {
          unsigned int
            alpha,
            pixel;