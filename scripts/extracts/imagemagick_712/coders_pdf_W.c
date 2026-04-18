// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 23/30



        /*
          Write ICC profile.
        */
        (void) FormatLocaleString(buffer,MagickPathExtent,
          "[/ICCBased %.20g 0 R]\n",(double) object+1);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,"endobj\n");
        xref[object++]=TellBlob(image);
        (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",
          (double) object);
        (void) WriteBlobString(image,buffer);
        (void) FormatLocaleString(buffer,MagickPathExtent,"<<\n/N %.20g\n"
          "/Filter /ASCII85Decode\n/Length %.20g 0 R\n/Alternate /%s\n>>\n"
          "stream\n",(double) channels,(double) object+1,device);
        (void) WriteBlobString(image,buffer);
        offset=TellBlob(image);
        Ascii85Initialize(image);
        r=GetStringInfoDatum(icc_profile);
        for (i=0; i < (ssize_t) GetStringInfoLength(icc_profile); i++)
          Ascii85Encode(image,(unsigned char) *r++);
        Ascii85Flush(image);
        offset=TellBlob(image)-offset-1;
        (void) WriteBlobString(image,"endstream\n");
        (void) WriteBlobString(image,"endobj\n");
        /*
          Write Length object.
        */
        xref[object++]=TellBlob(image);
        (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",
          (double) object);
        (void) WriteBlobString(image,buffer);
        (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g\n",(double)
          offset);
        (void) WriteBlobString(image,buffer);
      }
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Thumb object.
    */
    SetGeometry(image,&geometry);
    thumbnail=IsStringTrue(GetImageOption(image_info,"pdf:thumbnail"));
    if (thumbnail == MagickFalse)
      (void) ParseMetaGeometry("1x1+0+0>",&geometry.x,&geometry.y,
        &geometry.width,&geometry.height);
    else
      (void) ParseMetaGeometry("106x106+0+0>",&geometry.x,&geometry.y,
        &geometry.width,&geometry.height);
    tile_image=ThumbnailImage(image,geometry.width,geometry.height,exception);
    if (tile_image == (Image *) NULL)
      {
        xref=(MagickOffsetType *) RelinquishMagickMemory(xref);
        (void) CloseBlob(image);
        return(MagickFalse);
      }
    xref[object++]=TellBlob(image);
    (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
      object);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"<<\n");
    switch (compression)
    {
      case NoCompression:
      {
        (void) FormatLocaleString(buffer,MagickPathExtent,CFormat,
          "ASCII85Decode");
        break;
      }
      case JPEGCompression:
      {
        (void) FormatLocaleString(buffer,MagickPathExtent,CFormat,"DCTDecode");
        if (tile_image->colorspace != CMYKColorspace)
          break;
        (void) WriteBlobString(image,buffer);
        (void) CopyMagickString(buffer,"/Decode [1 0 1 0 1 0 1 0]\n",
          MagickPathExtent);
        break;
      }
      case JPEG2000Compression:
      {
        (void) FormatLocaleString(buffer,MagickPathExtent,CFormat,"JPXDecode");
        if (tile_image->colorspace != CMYKColorspace)
          break;
        (void) WriteBlobString(image,buffer);
        (void) CopyMagickString(buffer,"/Decode [1 0 1 0 1 0 1 0]\n",
          MagickPathExtent);
        break;
      }
      case LZWCompression:
      {
        (void) FormatLocaleString(buffer,MagickPathExtent,CFormat,"LZWDecode");
        break;
      }
      case ZipCompression:
      {
        (void) FormatLocaleString(buffer,MagickPathExtent,CFormat,
          "FlateDecode");
        break;
      }
      case FaxCompression:
      case Group4Compression:
      {
        (void) CopyMagickString(buffer,"/Filter [ /CCITTFaxDecode ]\n",
          MagickPathExtent);
        (void) WriteBlobString(image,buffer);
        (void) FormatLocaleString(buffer,MagickPathExtent,"/DecodeParms [ "
          "<< /K %s /BlackIs1 false /Columns %.20g /Rows %.20g >> ]\n",
          CCITTParam,(double) tile_image->columns,(double) tile_image->rows);
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
    (void) FormatLocaleString(buffer,MagickPathExtent,"/Width %.20g\n",(double)
      tile_image->columns);
    (void) WriteBlobString(image,buffer);
    (void) FormatLocaleString(buffer,MagickPathExtent,"/Height %.20g\n",(double)
      tile_image->rows);
    (void) WriteBlobString(image,buffer);
    (void) FormatLocaleString(buffer,MagickPathExtent,
      "/ColorSpace %.20g 0 R\n",(double) object-
      (icc_profile != (StringInfo *) NULL ? 3 : 1));
    (void) WriteBlobString(image,buffer);
    (void) FormatLocaleString(buffer,MagickPathExtent,
      "/BitsPerComponent %d\n",(compression == FaxCompression) ||
      (compression == Group4Compression) ? 1 : 8);
    (void) WriteBlobString(image,buffer);
    (