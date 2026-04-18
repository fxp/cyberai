// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 15/34



          pad=(ssize_t) samples_per_pixel;
          if (samples_per_pixel > 2)
            {
              if (image->colorspace == CMYKColorspace)
                {
                  pad-=4;
                  image_quantum_type=CMYKQuantum;
                  if (image->alpha_trait != UndefinedPixelTrait)
                    {
                      pad--;
                      image_quantum_type=CMYKAQuantum;
                    }
                }
              else
                {
                  pad-=3;
                  image_quantum_type=RGBQuantum;
                  if (image->alpha_trait != UndefinedPixelTrait)
                    {
                      pad--;
                      image_quantum_type=RGBAQuantum;
                    }
                }
            }
          else
            {
              pad--;
              if (image->alpha_trait != UndefinedPixelTrait)
                {
                  if (samples_per_pixel == 1)
                    image_quantum_type=AlphaQuantum;
                  else
                    {
                      pad--;
                      if (image->storage_class == PseudoClass)
                        image_quantum_type=IndexAlphaQuantum;
                      else
                        image_quantum_type=GrayAlphaQuantum;
                    }
                }
            }
          if (pad < 0)
            ThrowTIFFException(CorruptImageError,"CorruptImage");
          if (pad > 0)
            {
              status=SetQuantumPad(image,quantum_info,(size_t) pad*
                ((bits_per_sample+7) >> 3));
              if (status == MagickFalse)
                ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
            }
        }
    if (exception->severity < ErrorException)
      switch (method)
      {
        case ReadYCCKMethod:
        {
          /*
            Convert YCC TIFF image.
          */
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            Quantum
              *magick_restrict q;

            ssize_t
              x;

            unsigned char
              *p;

            tiff_status=TIFFReadPixels(tiff,0,y,(char *) pixels);
            if (tiff_status == -1)
              break;
            q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
            if (q == (Quantum *) NULL)
              break;
            p=pixels;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              SetPixelCyan(image,ScaleCharToQuantum(ClampYCC((double) *p+
                (1.402*(double) *(p+2))-179.456)),q);
              SetPixelMagenta(image,ScaleCharToQuantum(ClampYCC((double) *p-
                (0.34414*(double) *(p+1))-(0.71414*(double ) *(p+2))+
                135.45984)),q);
              SetPixelYellow(image,ScaleCharToQuantum(ClampYCC((double) *p+
                (1.772*(double) *(p+1))-226.816)),q);
              SetPixelBlack(image,ScaleCharToQuantum((unsigned char) *(p+3)),q);
              q+=(ptrdiff_t) GetPixelChannels(image);
              p+=(ptrdiff_t) 4;
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
          break;
        }
        case ReadStripMethod:
        {
          size_t
            count,
            extent,
            length,
            stride,
            strip_size;

          uint32_t
            strip_id;

          unsigned char
            *p,
            *strip_pixels;

          /*
            Convert stripped TIFF image.
          */
          strip_size=(size_t) TIFFStripSize(tiff);
          stride=(ssize_t) TIFFVStripSize(tiff,1);
          length=GetQuantumExtent(image,quantum_info,image_quantum_type);
          if (HeapOverflowSanityCheckGetSize(rows_per_strip,MagickMax(stride,length),&count) != MagickFalse)
            ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
          extent=MagickMax(strip_size,count);
          strip_pixels=(unsigned char *) AcquireQuantumMemory(extent,
            sizeof(*strip_pixels));
          if (strip_pixels == (unsigned char *) NULL)
            ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
          (void) memset(strip_pixels,0,extent*sizeof(*strip_pixels));
          strip_id=0;
          p=strip_pixels;
          for (i=0; i < (ssize_t) samples_per_pixel; i++)
          {
            QuantumType
              quantum_type = image_quantum_type;

            size_t
              rows_remaining;

            tmsize_t
              size = 0;