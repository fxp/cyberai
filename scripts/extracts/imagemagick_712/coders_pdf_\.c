// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 28/30

obByte(image,ScaleQuantumToChar(
            ClampToQuantum(image->colormap[i].blue)));
        }
        if (compression == NoCompression)
          Ascii85Flush(image);
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
    /*
      Write softmask object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
      object);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"<<\n");
    if ((image->alpha_trait & BlendPixelTrait) == 0)
      (void) WriteBlobString(image,">>\n");
    else
      {
        (void) WriteBlobString(image,"/Type /XObject\n");
        (void) WriteBlobString(image,"/Subtype /Image\n");
        (void) FormatLocaleString(buffer,MagickPathExtent,"/Name /Ma%.20g\n",
          (double) image->scene);
        (void) WriteBlobString(image,buffer);
        switch (compression)
        {
          case NoCompression:
          {
            (void) FormatLocaleString(buffer,MagickPathExtent,CFormat,
              "ASCII85Decode");
            break;
          }
          case LZWCompression:
          {
            (void) FormatLocaleString(buffer,MagickPathExtent,CFormat,
              "LZWDecode");
            break;
          }
          case ZipCompression:
          {
            (void) FormatLocaleString(buffer,MagickPathExtent,CFormat,
              "FlateDecode");
            break;
          }
          default:
          {
            (void) FormatLocaleString(buffer,MagickPathExtent,CFormat,
              "RunLengthDecode");
            break;
          }
        }
        (void) WriteBlobString(image,buffer);
        (void) FormatLocaleString(buffer,MagickPathExtent,"/Width %.20g\n",
          (double) image->columns);
        (void) WriteBlobString(image,buffer);
        (void) FormatLocaleString(buffer,MagickPathExtent,"/Height %.20g\n",
          (double) image->rows);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,"/ColorSpace /DeviceGray\n");
        (void) FormatLocaleString(buffer,MagickPathExtent,
          "/BitsPerComponent %d\n",(compression == FaxCompression) ||
          (compression == Group4Compression) ? 1 : 8);
        (void) WriteBlobString(image,buffer);
        (void) FormatLocaleString(buffer,MagickPathExtent,"/Length %.20g 0 R\n",
          (double) object+1);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,">>\n");
        (void) WriteBlobString(image,"stream\n");
        offset=TellBlob(image);
        number_pixels=(MagickSizeType) image->columns*image->rows;
        switch (compression)
        {
          case RLECompression:
          default:
          {
            MemoryInfo
              *pixel_info;