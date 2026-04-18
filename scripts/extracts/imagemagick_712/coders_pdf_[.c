// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 27/30



              /*
                Allocate pixel array.
              */
              length=(size_t) number_pixels;
              pixel_info=AcquireVirtualMemory(length,sizeof(*pixels));
              if (pixel_info == (MemoryInfo *) NULL)
                {
                  tile_image=DestroyImage(tile_image);
                  ThrowPDFException(ResourceLimitError,
                    "MemoryAllocationFailed");
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
                  *q++=(unsigned char) ((ssize_t) GetPixelIndex(tile_image,p));
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
                Dump uncompressed PseudoColor packets.
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
                  Ascii85Encode(image,(unsigned char) ((ssize_t)
                    GetPixelIndex(tile_image,p)));
                  p+=(ptrdiff_t) GetPixelChannels(image);
                }
              }
              Ascii85Flush(image);
              break;
            }
          }
        }
    tile_image=DestroyImage(tile_image);
    offset=TellBlob(image)-offset;
    (void) WriteBlobString(image,"\nendstream\n");
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Length object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
      object);
    (void) WriteBlobString(image,buffer);
    (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g\n",(double)
      offset);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"endobj\n");
    xref[object++]=TellBlob(image);
    (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
      object);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"<<\n");
    if ((image->storage_class == DirectClass) || (image->colors > 256) ||
        (compression == FaxCompression) || (compression == Group4Compression))
      (void) WriteBlobString(image,">>\n");
    else
      {
        /*
          Write Colormap object.
        */
        if (compression == NoCompression)
          (void) WriteBlobString(image,"/Filter [ /ASCII85Decode ]\n");
        (void) FormatLocaleString(buffer,MagickPathExtent,"/Length %.20g 0 R\n",
          (double) object+1);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,">>\n");
        (void) WriteBlobString(image,"stream\n");
        offset=TellBlob(image);
        if (compression == NoCompression)
          Ascii85Initialize(image);
        for (i=0; i < (ssize_t) image->colors; i++)
        {
          if (compression == NoCompression)
            {
              Ascii85Encode(image,ScaleQuantumToChar(ClampToQuantum(
                image->colormap[i].red)));
              Ascii85Encode(image,ScaleQuantumToChar(ClampToQuantum(
                image->colormap[i].green)));
              Ascii85Encode(image,ScaleQuantumToChar(ClampToQuantum(
                image->colormap[i].blue)));
              continue;
            }
          (void) WriteBlobByte(image,ScaleQuantumToChar(
            ClampToQuantum(image->colormap[i].red)));
          (void) WriteBlobByte(image,ScaleQuantumToChar(
            ClampToQuantum(image->colormap[i].green)));
          (void) WriteBl