// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 22/30



              /*
                Allocate pixel array.
              */
              length=(size_t) number_pixels;
              pixel_info=AcquireVirtualMemory(length,sizeof(*pixels));
              if (pixel_info == (MemoryInfo *) NULL)
                ThrowPDFException(ResourceLimitError,"MemoryAllocationFailed");
              pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
              /*
                Dump Runlength encoded pixels.
              */
              q=pixels;
              for (y=0; y < (ssize_t) image->rows; y++)
              {
                p=GetVirtualPixels(image,0,y,image->columns,1,exception);
                if (p == (const Quantum *) NULL)
                  break;
                for (x=0; x < (ssize_t) image->columns; x++)
                {
                  *q++=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
                  p+=(ptrdiff_t) GetPixelChannels(image);
                }
                if (image->previous == (Image *) NULL)
                  {
                    status=SetImageProgress(image,SaveImageTag,
                      (MagickOffsetType) y,image->rows);
                    if (status == MagickFalse)
                      break;
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
              for (y=0; y < (ssize_t) image->rows; y++)
              {
                p=GetVirtualPixels(image,0,y,image->columns,1,exception);
                if (p == (const Quantum *) NULL)
                  break;
                for (x=0; x < (ssize_t) image->columns; x++)
                {
                  Ascii85Encode(image,(unsigned char) ((ssize_t)
                    GetPixelIndex(image,p)));
                  p+=(ptrdiff_t) GetPixelChannels(image);
                }
                if (image->previous == (Image *) NULL)
                  {
                    status=SetImageProgress(image,SaveImageTag,
                      (MagickOffsetType) y,image->rows);
                    if (status == MagickFalse)
                      break;
                  }
              }
              Ascii85Flush(image);
              break;
            }
          }
        }
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
    /*
      Write Colorspace object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
      object);
    (void) WriteBlobString(image,buffer);
    device="DeviceRGB";
    channels=0;
    if (image->colorspace == CMYKColorspace)
      {
        device="DeviceCMYK";
        channels=4;
      }
    else
      if ((compression == FaxCompression) ||
          (compression == Group4Compression) ||
          (IsImageGray(image) != MagickFalse))
        {
          device="DeviceGray";
          channels=1;
        }
      else
        if ((image->storage_class == DirectClass) ||
            (image->colors > 256) || (compression == JPEGCompression) ||
            (compression == JPEG2000Compression))
          channels=3;
    if (icc_profile == (StringInfo *) NULL)
      {
        if (channels != 0)
          (void) FormatLocaleString(buffer,MagickPathExtent,"/%s\n",device);
        else
          (void) FormatLocaleString(buffer,MagickPathExtent,
            "[ /Indexed /%s %.20g %.20g 0 R ]\n",device,(double) image->colors-1,
            (double) object+3);
        (void) WriteBlobString(image,buffer);
      }
    else
      {
        const unsigned char
          *r;