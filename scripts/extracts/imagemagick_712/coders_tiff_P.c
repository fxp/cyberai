// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 16/34



            switch (i)
            {
              case 0:
              {
                if (interlace == PLANARCONFIG_SEPARATE)
                  quantum_type=RedQuantum;
                break;
              }
              case 1: quantum_type=GreenQuantum; break;
              case 2: quantum_type=BlueQuantum; break;
              case 3:
              {
                quantum_type=AlphaQuantum;
                if (image->colorspace == CMYKColorspace)
                  quantum_type=BlackQuantum;
                break;
              }
              case 4:
              {
                if (image->colorspace == CMYKColorspace)
                  {
                    quantum_type=AlphaQuantum;
                    break;
                  }
                magick_fallthrough;
              }
              default:
              {
                if (quantum_type == MultispectralQuantum)
                  {
                    if (image->colorspace == CMYKColorspace)
                      (void) SetQuantumMetaChannel(image,quantum_info,i-5);
                    else
                      (void) SetQuantumMetaChannel(image,quantum_info,i-4);
                  }
                break;
              }
            }
            rows_remaining=0;
            for (y=0; y < (ssize_t) image->rows; y++)
            {
              Quantum
                *magick_restrict q;

              q=GetAuthenticPixels(image,0,y,image->columns,1,exception);
              if (q == (Quantum *) NULL)
                break;
              if (rows_remaining == 0)
                {
                  size=TIFFReadEncodedStrip(tiff,strip_id,strip_pixels,
                    strip_size);
                  if (size == -1)
                    break;
                  rows_remaining=rows_per_strip;
                  p=strip_pixels;
                  strip_id++;
                }
              (void) ImportQuantumPixels(image,(CacheView *) NULL,
                quantum_info,quantum_type,p,exception);
              p+=(ptrdiff_t) stride;
              rows_remaining--;
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
              if (image->previous == (Image *) NULL)
                {
                  status=SetImageProgress(image,LoadImageTag,(MagickOffsetType)
                    y,image->rows);
                  if (status == MagickFalse)
                    break;
                }
            }
            if ((size == -1) || ((samples_per_pixel > 1) &&
                (interlace != PLANARCONFIG_SEPARATE)))
              break;
          }
          (void) SetQuantumMetaChannel(image,quantum_info,-1);
          strip_pixels=(unsigned char *) RelinquishMagickMemory(strip_pixels);
          break;
        }
        case ReadTileMethod:
        {
          size_t
            count,
            extent,
            length,
            stride,
            tile_size;

          uint32
            columns,
            rows;

          unsigned char
            *p,
            *tile_pixels;

          /*
            Convert tiled TIFF image.
          */
          if ((TIFFGetField(tiff,TIFFTAG_TILEWIDTH,&columns) != 1) ||
              (TIFFGetField(tiff,TIFFTAG_TILELENGTH,&rows) != 1))
            ThrowTIFFException(CoderError,"ImageIsNotTiled");
          if (HeapOverflowSanityCheckGetSize(columns,rows,&count) != MagickFalse)
            ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
          number_pixels=(MagickSizeType) count;
          if (HeapOverflowSanityCheck(rows,sizeof(*tile_pixels)) != MagickFalse)
            ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
          tile_size=(size_t) TIFFTileSize(tiff);
          stride=(size_t) TIFFTileRowSize(tiff);
          length=GetQuantumExtent(image,quantum_info,image_quantum_type);
          if (HeapOverflowSanityCheckGetSize(rows,MagickMax(stride,length),&count) != MagickFalse)
            ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
          extent=MagickMax(tile_size,count);
          tile_pixels=(unsigned char *) AcquireQuantumMemory(extent,
            sizeof(*tile_pixels));
          if (tile_pixels == (unsigned char *) NULL)
            ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
          (void) memset(tile_pixels,0,extent*sizeof(*tile_pixels));
          for (i=0; i < (ssize_t) samples_per_pixel; i++)
          {
            QuantumType
              quantum_type = image_quantum_type;

            tmsize_t
              size = 0;