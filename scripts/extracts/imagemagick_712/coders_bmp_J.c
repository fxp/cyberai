// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 10/24



        /*
          Read BMP raster colormap.
        */
        if (image->debug != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "  Reading colormap of %.20g colors",(double) image->colors);
        if (AcquireImageColormap(image,image->colors,exception) == MagickFalse)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        bmp_colormap=(unsigned char *) AcquireQuantumMemory((size_t)
          image->colors,4*sizeof(*bmp_colormap));
        if (bmp_colormap == (unsigned char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        if ((bmp_info.size == 12) || (bmp_info.size == 64))
          packet_size=3;
        else
          packet_size=4;
        offset=SeekBlob(image,start_position+14+bmp_info.size,SEEK_SET);
        if (offset < 0)
          {
            bmp_colormap=(unsigned char *) RelinquishMagickMemory(bmp_colormap);
            ThrowReaderException(CorruptImageError,"ImproperImageHeader");
          }
        count=ReadBlob(image,packet_size*image->colors,bmp_colormap);
        if (count != (ssize_t) (packet_size*image->colors))
          {
            bmp_colormap=(unsigned char *) RelinquishMagickMemory(bmp_colormap);
            ThrowReaderException(CorruptImageError,
              "InsufficientImageDataInFile");
          }
        p=bmp_colormap;
        for (i=0; i < (ssize_t) image->colors; i++)
        {
          image->colormap[i].blue=(MagickRealType) ScaleCharToQuantum(*p++);
          image->colormap[i].green=(MagickRealType) ScaleCharToQuantum(*p++);
          image->colormap[i].red=(MagickRealType) ScaleCharToQuantum(*p++);
          if (packet_size == 4)
            p++;
        }
        bmp_colormap=(unsigned char *) RelinquishMagickMemory(bmp_colormap);
      }
    /*
      Read image data.
    */
    if (bmp_info.offset_bits == offset_bits)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    offset_bits=bmp_info.offset_bits;
    offset=SeekBlob(image,start_position+bmp_info.offset_bits,SEEK_SET);
    if (offset < 0)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    if (bmp_info.compression == BI_RLE4)
      bmp_info.bits_per_pixel<<=1;
    if (HeapOverflowSanityCheckGetSize(image->columns,bmp_info.bits_per_pixel,&extent) != MagickFalse)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    if (HeapOverflowSanityCheckGetSize(4,((extent+31)/32),&bytes_per_line) != MagickFalse)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    if (HeapOverflowSanityCheckGetSize(bytes_per_line,image->rows,&length) != MagickFalse)
      ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
    if ((MagickSizeType) (length/256) > blob_size)
      ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
    extent=MagickMax(bytes_per_line,image->columns+1UL);
    if (HeapOverflowSanityCheck(extent,sizeof(*pixels)) != MagickFalse)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    pixel_info=AcquireVirtualMemory(image->rows,extent*sizeof(*pixels));
    if (pixel_info == (MemoryInfo *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
    if ((bmp_info.compression == BI_RGB) ||
        (bmp_info.compression == BI_BITFIELDS) ||
        (bmp_info.compression == BI_ALPHABITFIELDS))
      {
        if (image->debug != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "  Reading pixels (%.20g bytes)",(double) length);
        count=ReadBlob(image,length,pixels);
        if (count != (ssize_t) length)
          {
            pixel_info=RelinquishVirtualMemory(pixel_info);
            ThrowReaderException(CorruptImageError,
              "InsufficientImageDataInFile");
          }
      }
    else
      {
        /*
          Convert run-length encoded raster pixels.
        */
        status=DecodeImage(image,bmp_info.compression,pixels,
          image->columns*image->rows);
        if (status == MagickFalse)
          {
            pixel_info=RelinquishVirtualMemory(pixel_info);
            ThrowReaderException(CorruptImageError,
              "UnableToRunlengthDecodeImage");
          }
      }
    /*
      Convert BMP raster image to pixel packets.
    */
    if (bmp_info.compression == BI_RGB)
      {
        /*
          We should ignore the alpha value in BMP3 files but there have been
          reports about 32 bit files with alpha. We do a quick check to see if
          the alpha channel contains a value that is not zero (default value).
          If we find a non zero value we assume the program that wrote the file
          wants to use the alpha channel.
        */
        if (((image->alpha_trait & BlendPixelTrait) == 0) &&
            (bmp_info.size == 40) && (bmp_info.bits_per_pixel == 32))
          {
 