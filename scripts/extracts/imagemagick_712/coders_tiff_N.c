// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 14/34



            option=GetImageOption(image_info,"tiff:assume-alpha");
            if (IsStringTrue(option) != MagickFalse)
              image->alpha_trait=BlendPixelTrait;
          }
        if (image->alpha_trait != UndefinedPixelTrait)
          extra_samples--;
        if (extra_samples > 0)
          {
            if (SetPixelMetaChannels(image,extra_samples,exception) == MagickFalse)
              ThrowTIFFException(OptionError,"SetPixelMetaChannelsFailure");
          }
      }
    if (image->alpha_trait != UndefinedPixelTrait)
      {
        if (quantum_info->alpha_type == UndefinedQuantumAlpha)
          (void) SetImageProperty(image,"tiff:alpha","unspecified",exception);
        (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
      }
    method=ReadGenericMethod;
    rows_per_strip=(uint32) image->rows;
    if (TIFFGetField(tiff,TIFFTAG_ROWSPERSTRIP,&rows_per_strip) == 1)
      {
        char
          buffer[MagickPathExtent];

        (void) FormatLocaleString(buffer,MagickPathExtent,"%u",
          (unsigned int) rows_per_strip);
        (void) SetImageProperty(image,"tiff:rows-per-strip",buffer,exception);
        method=ReadStripMethod;
        if (rows_per_strip > (uint32) image->rows)
          rows_per_strip=(uint32) image->rows;
      }
    else if (image->depth > 8)
      method=ReadStripMethod;
    if (TIFFIsTiled(tiff) != MagickFalse)
      {
        uint32
          columns,
          rows;

        if ((TIFFGetField(tiff,TIFFTAG_TILEWIDTH,&columns) != 1) ||
            (TIFFGetField(tiff,TIFFTAG_TILELENGTH,&rows) != 1))
          ThrowTIFFException(CoderError,"ImageIsNotTiled");
        if ((AcquireMagickResource(WidthResource,columns) == MagickFalse) ||
            (AcquireMagickResource(HeightResource,rows) == MagickFalse))
          ThrowTIFFException(ImageError,"WidthOrHeightExceedsLimit");
        method=ReadTileMethod;
      }
    if ((photometric == PHOTOMETRIC_LOGLUV) ||
        (compress_tag == COMPRESSION_CCITTFAX3))
      method=ReadGenericMethod;
    if (image->compression == JPEGCompression)
      method=GetJPEGMethod(image,tiff,photometric,bits_per_sample,
        samples_per_pixel);
#if defined(WORDS_BIGENDIAN)
    (void) SetQuantumEndian(image,quantum_info,MSBEndian);
#else
    (void) SetQuantumEndian(image,quantum_info,LSBEndian);
#endif
    scanline_size=TIFFScanlineSize(tiff);
    if (scanline_size <= 0)
      ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
    number_pixels=MagickMax((MagickSizeType) (image->columns*samples_per_pixel*
      pow(2.0,ceil(log(bits_per_sample)/log(2.0)))),image->columns*
      rows_per_strip);
    if ((double) scanline_size > 1.5*number_pixels)
      ThrowTIFFException(CorruptImageError,"CorruptImage");
    number_pixels=MagickMax((MagickSizeType) scanline_size,number_pixels);
    pixel_info=AcquireVirtualMemory((size_t) number_pixels,sizeof(uint32));
    if (pixel_info == (MemoryInfo *) NULL)
      ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
    pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
    (void) memset(pixels,0,(size_t) number_pixels*sizeof(uint32));
    image_quantum_type=GrayQuantum;
    if (image->storage_class == PseudoClass)
      image_quantum_type=IndexQuantum;
    if (image->number_meta_channels != 0)
      {
        image_quantum_type=MultispectralQuantum;
        (void) SetQuantumPad(image,quantum_info,0);
      }
    else
      if (interlace != PLANARCONFIG_SEPARATE)
        {
          ssize_t
            pad;