/* ===== EXTRACT: tiff.c ===== */
/* Function: ReadTIFFImage (part F) */
/* Lines: 1771–1888 */

    if ((double) scanline_size > 1.5*number_pixels)
      ThrowTIFFException(CorruptImageError,"CorruptImage");
    number_pixels=MagickMax((MagickSizeType) scanline_size,number_pixels);
    pixel_info=AcquireVirtualMemory(number_pixels,sizeof(uint32));
    if (pixel_info == (MemoryInfo *) NULL)
      ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
    pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
    (void) memset(pixels,0,number_pixels*sizeof(uint32));
    quantum_type=GrayQuantum;
    if (image->storage_class == PseudoClass)
      quantum_type=IndexQuantum;
    if (image->number_meta_channels != 0)
      {
        quantum_type=MultispectralQuantum;
        (void) SetQuantumPad(image,quantum_info,0);
      }
    else
      if (interlace != PLANARCONFIG_SEPARATE)
        {
          ssize_t
            pad;

          pad=(ssize_t) samples_per_pixel;
          if (samples_per_pixel > 2)
            {
              if (image->colorspace == CMYKColorspace)
                {
                  pad-=4;
                  quantum_type=CMYKQuantum;
                  if (image->alpha_trait != UndefinedPixelTrait)
                    {
                      pad--;
                      quantum_type=CMYKAQuantum;
                    }
                }
              else
                {
                  pad-=3;
                  quantum_type=RGBQuantum;
                  if (image->alpha_trait != UndefinedPixelTrait)
                    {
                      pad--;
                      quantum_type=RGBAQuantum;
                    }
                }
            }
          else
            {
              pad--;
              if (image->alpha_trait != UndefinedPixelTrait)
                {
                  if (samples_per_pixel == 1)
                    quantum_type=AlphaQuantum;
                  else
                    {
                      pad--;
                      if (image->storage_class == PseudoClass)
                        quantum_type=IndexAlphaQuantum;
                      else
                        quantum_type=GrayAlphaQuantum;
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
