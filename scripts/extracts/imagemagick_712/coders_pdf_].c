// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 29/30



            /*
              Allocate pixel array.
            */
            length=(size_t) number_pixels;
            pixel_info=AcquireVirtualMemory(length,4*sizeof(*pixels));
            if (pixel_info == (MemoryInfo *) NULL)
              {
                image=DestroyImage(image);
                ThrowPDFException(ResourceLimitError,"MemoryAllocationFailed");
              }
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
                *q++=ScaleQuantumToChar(GetPixelAlpha(image,p));
                p+=(ptrdiff_t) GetPixelChannels(image);
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
                Ascii85Encode(image,ScaleQuantumToChar(GetPixelAlpha(image,p)));
                p+=(ptrdiff_t) GetPixelChannels(image);
              }
            }
            Ascii85Flush(image);
            break;
          }
        }
        offset=TellBlob(image)-offset;
        (void) WriteBlobString(image,"\nendstream\n");
      }
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
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    status=SetImageProgress(image,SaveImagesTag,scene++,number_scenes);
    if (status == MagickFalse)
      break;
  } while (image_info->adjoin != MagickFalse);
  /*
    Write Metadata object.
  */
  xref[object++]=TellBlob(image);
  info_id=object;
  (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
    object);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"<<\n");
  WritePDFValue(image,"Title",GetPDFTitle(image_info,basename),is_pdfa);
  WritePDFValue(image,"Author",GetPDFAuthor(image_info),is_pdfa);
  WritePDFValue(image,"Creator",GetPDFCreator(image_info),is_pdfa);
  WritePDFValue(image,"Producer",GetPDFProducer(image_info),is_pdfa);
  WritePDFValue(image,"Subject",GetPDFSubject(image_info),is_pdfa);
  WritePDFValue(image,"Keywords",GetPDFKeywords(image_info),is_pdfa);
  seconds=GetPdfCreationDate(image_info,image);
  GetMagickUTCTime(&seconds,&utc_time);
  (void) FormatLocaleString(temp,MagickPathExtent,"D:%04d%02d%02d%02d%02d%02d",
    utc_time.tm_year+1900,utc_time.tm_mon+1,utc_time.tm_mday,
    utc_time.tm_hour,utc_time.tm_min,utc_time.tm_sec);
  (void) FormatLocaleString(buffer,MagickPathExtent,"/CreationDate (%s)\n",
    temp);
  (void) WriteBlobString(image,buffer);
  seconds=GetPdfModDate(image_info,image);
  GetMagickUTCTime(&seconds,&utc_time);
  (void) FormatLocaleString(temp,MagickPathExtent,"D:%04d%02d%02d%02d%02d%02d",
    utc_time.tm_year+1900,utc_time.tm_mon+1,utc_time.tm_mday,
    utc_time.tm_hour,utc_time.tm_min,utc_time.tm_sec);
  (void) FormatLocaleString(buffer,MagickPathExtent,"/ModDate (%s)\n",temp);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,">>\n");
  (void) WriteBlobString(image,"endobj\n");
  /*
    Write Xref object.
  */
  offset=TellBlob(image)-xref[0]+((is_pdfa != MagickFalse) ? 6 : 0)+9;
  (void) WriteBlobString(image,"xref\n");
  (void) FormatLocaleString(buffer,MagickPathExtent,"0 %.20g\n",(double)
    object+1);
  (void) Wr