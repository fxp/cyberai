// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 6/24



    /*
      Verify BMP identifier.
    */
    start_position=TellBlob(image)-2;
    bmp_info.ba_offset=0;
    while (LocaleNCompare((char *) magick,"BA",2) == 0)
    {
      bmp_info.file_size=ReadBlobLSBLong(image);
      bmp_info.ba_offset=ReadBlobLSBLong(image);
      bmp_info.offset_bits=ReadBlobLSBLong(image);
      count=ReadBlob(image,2,magick);
      if (count != 2)
        break;
    }
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  Magick: %c%c",
        magick[0],magick[1]);
    if ((count != 2) || ((LocaleNCompare((char *) magick,"BM",2) != 0) &&
        (LocaleNCompare((char *) magick,"CI",2) != 0)))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    bmp_info.file_size=ReadBlobLSBLong(image);
    (void) ReadBlobLSBLong(image);
    bmp_info.offset_bits=ReadBlobLSBLong(image);
    bmp_info.size=ReadBlobLSBLong(image);
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  BMP header size: %u",bmp_info.size);
    if (LocaleNCompare((char *) magick,"CI",2) == 0)
      {
        if ((bmp_info.size != 12) && (bmp_info.size != 40) &&
            (bmp_info.size != 64))
          ThrowReaderException(CorruptImageError,"ImproperImageHeader");
      }
    if (bmp_info.size > 124)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    /*
      Get option to bypass file size check
    */
    ignore_filesize=IsStringTrue(GetImageOption(image_info,
      "bmp:ignore-filesize"));
    if ((ignore_filesize == MagickFalse) && (bmp_info.file_size != 0) &&
        ((MagickSizeType) bmp_info.file_size > GetBlobSize(image)))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    if (bmp_info.offset_bits < bmp_info.size)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    bmp_info.offset_bits=MagickMax(14+bmp_info.size,bmp_info.offset_bits);
    profile_data=0;
    profile_size=0;
    if (bmp_info.size == 12)
      {
        /*
          OS/2 BMP image file.
        */
        (void) CopyMagickString(image->magick,"BMP2",MagickPathExtent);
        bmp_info.width=(ssize_t) ((short) ReadBlobLSBShort(image));
        bmp_info.height=(ssize_t) ((short) ReadBlobLSBShort(image));
        bmp_info.planes=ReadBlobLSBShort(image);
        bmp_info.bits_per_pixel=ReadBlobLSBShort(image);
        bmp_info.x_pixels=0;
        bmp_info.y_pixels=0;
        bmp_info.number_colors=0;
        bmp_info.compression=BI_RGB;
        bmp_info.image_size=0;
        bmp_info.alpha_mask=0;
        if (image->debug != MagickFalse)
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Format: OS/2 Bitmap");
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Geometry: %.20gx%.20g",(double) bmp_info.width,(double)
              bmp_info.height);
          }
      }
    else
      {
        /*
          Microsoft Windows BMP image file.
        */
        switch (bmp_info.size)
        {
          case 40: case 52: case 56: case 64: case 78: case 108: case 124:
          {
            break;
          }
          default:
          {
            if (bmp_info.size < 64)
              ThrowReaderException(CorruptImageError,"ImproperImageHeader");
            break;
          }
        }
        bmp_info.width=(ssize_t) ReadBlobLSBSignedLong(image);
        bmp_info.height=(ssize_t) ReadBlobLSBSignedLong(image);
        bmp_info.planes=ReadBlobLSBShort(image);
        bmp_info.bits_per_pixel=ReadBlobLSBShort(image);
        bmp_info.compression=ReadBlobLSBLong(image);
        if (bmp_info.size > 16)
          {
            bmp_info.image_size=ReadBlobLSBLong(image);
            bmp_info.x_pixels=ReadBlobLSBLong(image);
            bmp_info.y_pixels=ReadBlobLSBLong(image);
            bmp_info.number_colors=ReadBlobLSBLong(image);
            bmp_info.colors_important=ReadBlobLSBLong(image);
          }
        if (image->debug != MagickFalse)
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Format: MS Windows bitmap");
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Geometry: %.20gx%.20g",(double) bmp_info.width,(double)
              bmp_info.height);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Bits per pixel: %.20g",(double) bmp_info.bits_per_pixel);
            switch (bmp_info.compression)
            {
              case BI_RGB:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_RGB");
                break;
              }
              case BI_RLE4:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_RLE4");
                break;
              }
              case BI_RLE8:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickMo