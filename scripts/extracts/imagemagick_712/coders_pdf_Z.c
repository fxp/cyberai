// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 26/30



            /*
              Allocate pixel array.
            */
            length=(size_t) number_pixels;
            length*=tile_image->colorspace == CMYKColorspace ? 4UL : 3UL;
            pixel_info=AcquireVirtualMemory(length,4*sizeof(*pixels));
            if (pixel_info == (MemoryInfo *) NULL)
              {
                tile_image=DestroyImage(tile_image);
                ThrowPDFException(ResourceLimitError,"MemoryAllocationFailed");
              }
            pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
            /*
              Dump runlength encoded pixels.
            */
            q=pixels;
            for (y=0; y < (ssize_t) tile_image->rows; y++)
            {
              p=GetVirtualPixels(tile_image,0,y,tile_image->columns,1,
                exception);
              if (p == (const Quantum *) NULL)
                break;
              for (x=0; x < (ssize_t) tile_image->columns; x++)
              {
                *q++=ScaleQuantumToChar(GetPixelRed(tile_image,p));
                *q++=ScaleQuantumToChar(GetPixelGreen(tile_image,p));
                *q++=ScaleQuantumToChar(GetPixelBlue(tile_image,p));
                if (tile_image->colorspace == CMYKColorspace)
                  *q++=ScaleQuantumToChar(GetPixelBlack(tile_image,p));
                p+=(ptrdiff_t) GetPixelChannels(tile_image);
              }
            }
#if defined(MAGICKCORE_ZLIB_DELEGATE)
            if (compression == ZipCompression)
              status=ZLIBEncodeImage(image,length,pixels,exception);
            else
#endif
              if (compression == LZWCompression)
                status=LZWEncodeImage(image,length,pixels,exception);
              else
                status=PackbitsEncodeImage(image,length,pixels,exception);
            pixel_info=RelinquishVirtualMemory(pixel_info);
            if (status == MagickFalse)
              {
                tile_image=DestroyImage(tile_image);
                xref=(MagickOffsetType *) RelinquishMagickMemory(xref);
                (void) CloseBlob(image);
                return(MagickFalse);
              }
            break;
          }
          case NoCompression:
          {
            /*
              Dump uncompressed DirectColor packets.
            */
            Ascii85Initialize(image);
            for (y=0; y < (ssize_t) tile_image->rows; y++)
            {
              p=GetVirtualPixels(tile_image,0,y,tile_image->columns,1,
                exception);
              if (p == (const Quantum *) NULL)
                break;
              for (x=0; x < (ssize_t) tile_image->columns; x++)
              {
                Ascii85Encode(image,ScaleQuantumToChar(
                  GetPixelRed(tile_image,p)));
                Ascii85Encode(image,ScaleQuantumToChar(
                  GetPixelGreen(tile_image,p)));
                Ascii85Encode(image,ScaleQuantumToChar(
                  GetPixelBlue(tile_image,p)));
                if (image->colorspace == CMYKColorspace)
                  Ascii85Encode(image,ScaleQuantumToChar(
                    GetPixelBlack(tile_image,p)));
                p+=(ptrdiff_t) GetPixelChannels(tile_image);
              }
            }
            Ascii85Flush(image);
            break;
          }
        }
      else
        {
          /*
            Dump number of colors and colormap.
          */
          switch (compression)
          {
            case RLECompression:
            default:
            {
              MemoryInfo
                *pixel_info;