/* ===== EXTRACT: tiff.c ===== */
/* Function: ReadTIFFImage (part G) */
/* Lines: 1889–2028 */

                if (status == MagickFalse)
                  break;
              }
          }
          break;
        }
        case ReadStripMethod:
        {
          size_t
            extent,
            length;

          ssize_t
            stride,
            strip_id;

          tmsize_t
            strip_size;

          unsigned char
            *p,
            *strip_pixels;

          /*
            Convert stripped TIFF image.
          */
          strip_size=TIFFStripSize(tiff);
          stride=(ssize_t) TIFFVStripSize(tiff,1);
          length=GetQuantumExtent(image,quantum_info,quantum_type);
          extent=(size_t) MagickMax((size_t) strip_size,rows_per_strip*
            MagickMax((size_t) stride,length));
          strip_pixels=(unsigned char *) AcquireQuantumMemory(extent,
            sizeof(*strip_pixels));
          if (strip_pixels == (unsigned char *) NULL)
            ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
          (void) memset(strip_pixels,0,extent*sizeof(*strip_pixels));
          strip_id=0;
          p=strip_pixels;
          for (i=0; i < (ssize_t) samples_per_pixel; i++)
          {
            size_t
              rows_remaining;

            tmsize_t
              size = 0;

            switch (i)
            {
              case 0: break;
              case 1: quantum_type=GreenQuantum; break;
              case 2: quantum_type=BlueQuantum; break;
              case 3:
              {
                quantum_type=AlphaQuantum;
                if (image->colorspace == CMYKColorspace)
                  quantum_type=BlackQuantum;
                break;
              }
              case 4: quantum_type=AlphaQuantum; break;
              default:
                break;
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
          strip_pixels=(unsigned char *) RelinquishMagickMemory(strip_pixels);
          break;
        }
        case ReadTileMethod:
        {
          unsigned char
            *p;

          size_t
            extent,
            length;

          ssize_t
            stride;

          tmsize_t
            tile_size;

          uint32
            columns,
            rows;

          unsigned char
            *tile_pixels;

          /*
            Convert tiled TIFF image.
          */
          if ((TIFFGetField(tiff,TIFFTAG_TILEWIDTH,&columns) != 1) ||
              (TIFFGetField(tiff,TIFFTAG_TILELENGTH,&rows) != 1))
            ThrowTIFFException(CoderError,"ImageIsNotTiled");
          number_pixels=(MagickSizeType) columns*rows;
          if (HeapOverflowSanityCheck(rows,sizeof(*tile_pixels)) != MagickFalse)
            ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
          tile_size=TIFFTileSize(tiff);
          stride=(ssize_t) TIFFTileRowSize(tiff);
          length=GetQuantumExtent(image,quantum_info,quantum_type);
          extent=(size_t) MagickMax((size_t) tile_size,rows*
            MagickMax((size_t) stride,length));
          tile_pixels=(unsigned char *) AcquireQuantumMemory(extent,
            sizeof(*tile_pixels));
