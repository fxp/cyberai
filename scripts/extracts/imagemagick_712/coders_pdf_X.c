// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 24/30

void) FormatLocaleString(buffer,MagickPathExtent,"/Length %.20g 0 R\n",
      (double) object+1);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,">>\n");
    (void) WriteBlobString(image,"stream\n");
    offset=TellBlob(image);
    number_pixels=(MagickSizeType) tile_image->columns*tile_image->rows;
    if ((compression == FaxCompression) ||
        (compression == Group4Compression) ||
        (SetImageCoderGray(tile_image,exception) != MagickFalse))
      {
        switch (compression)
        {
          case FaxCompression:
          case Group4Compression:
          {
            if (LocaleCompare(CCITTParam,"0") == 0)
              {
                (void) HuffmanEncodeImage(image_info,image,tile_image,
                  exception);
                break;
              }
            (void) Huffman2DEncodeImage(image_info,image,tile_image,exception);
            break;
          }
          case JPEGCompression:
          {
            status=InjectImageBlob(image_info,image,tile_image,"jpeg",
              exception);
            if (status == MagickFalse)
              {
                tile_image=DestroyImage(tile_image);
                xref=(MagickOffsetType *) RelinquishMagickMemory(xref);
                (void) CloseBlob(image);
                return(MagickFalse);
              }
            break;
          }
          case JPEG2000Compression:
          {
            status=InjectImageBlob(image_info,image,tile_image,"jp2",exception);
            if (status == MagickFalse)
              {
                tile_image=DestroyImage(tile_image);
                xref=(MagickOffsetType *) RelinquishMagickMemory(xref);
                (void) CloseBlob(image);
                return(MagickFalse);
              }
            break;
          }
          case RLECompression:
          default:
          {
            MemoryInfo
              *pixel_info;