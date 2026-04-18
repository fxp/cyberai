// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 17/34



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
            for (y=0; y < (ssize_t) image->rows; y+=rows)
            {
              ssize_t
                x;

              size_t
                rows_remaining;

              rows_remaining=image->rows-(size_t) y;
              if ((ssize_t) (y+rows) < (ssize_t) image->rows)
                rows_remaining=rows;
              for (x=0; x < (ssize_t) image->columns; x+=columns)
              {
                size_t
                  columns_remaining,
                  row;

                columns_remaining=image->columns-(size_t) x;
                if ((x+(ssize_t) columns) < (ssize_t) image->columns)
                  columns_remaining=columns;
                size=TIFFReadTile(tiff,tile_pixels,(uint32_t) x,(uint32_t) y,
                  0,(uint16_t) i);
                if (size == -1)
                  break;
                p=tile_pixels;
                for (row=0; row < rows_remaining; row++)
                {
                  Quantum
                    *magick_restrict q;

                  q=GetAuthenticPixels(image,x,y+(ssize_t) row,
                    columns_remaining,1,exception);
                  if (q == (Quantum *) NULL)
                    break;
                  (void) ImportQuantumPixels(image,(CacheView *) NULL,
                    quantum_info,quantum_type,p,exception);
                  p+=(ptrdiff_t) stride;
                  if (SyncAuthenticPixels(image,exception) == MagickFalse)
                    break;
                }
              }
              if (size == -1)
                break;
            }
            if ((size == -1) || ((samples_per_pixel > 1) &&
                (interlace != PLANARCONFIG_SEPARATE)))
              break;
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) i,
                  samples_per_pixel);
                if (status == MagickFalse)
                  break;
              }
          }
          (void) SetQuantumMetaChannel(image,quantum_info,-1);
          tile_pixels=(unsigned char *) RelinquishMagickMemory(tile_pixels);
          break;
        }
        case ReadGenericMethod:
        default:
        {
          MemoryInfo
            *generic_info = (MemoryInfo * ) NULL;

          size_t
            count;

          uint32
            *p;

          /*
            Convert generic TIFF image.
          */
          (void) SetImageStorageClass(image,DirectClass,exception);
          if (HeapOverflowSanityCheckGetSize(image->rows,image->columns,&count) != MagickFalse)
            ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
          number_pixels=(MagickSizeType) count;
          generic_info=AcquireVirtualMemory(count,sizeof(*p));
          if (generic_info == (MemoryInfo *) NULL)
            ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
          p=(uint32 *) GetVirtualMemoryBlob(generic_info);
          (void) memset(p,0,count*sizeof(*p));
          tiff_status=TIFFReadRGBAImage(tiff,(uint32) image->columns,(uint32)
            image->rows,p,0);
          if (tiff_status == -1)
            {
              generic_info=RelinquishVirtualMemory(generic_info);
              break;
            }
          p+=(ptrdiff_t) (image->columns*image->rows)-1;
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            ssize_t
              x;

            Quantum
              *magick_restrict q;