/* ===== EXTRACT: tiff.c ===== */
/* Function: ReadTIFFImage (part E) */
/* Lines: 1652–1770 */

    if (sample_format == SAMPLEFORMAT_IEEEFP)
      status=SetQuantumFormat(image,quantum_info,FloatingPointQuantumFormat);
    if (status == MagickFalse)
      ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
    status=MagickTrue;
    switch (photometric)
    {
      case PHOTOMETRIC_MINISBLACK:
      {
        SetQuantumMinIsWhite(quantum_info,MagickFalse);
        break;
      }
      case PHOTOMETRIC_MINISWHITE:
      {
        SetQuantumMinIsWhite(quantum_info,MagickTrue);
        break;
      }
      default:
        break;
    }
    tiff_status=TIFFGetFieldDefaulted(tiff,TIFFTAG_EXTRASAMPLES,&extra_samples,
      &sample_info,sans);
    if (extra_samples == 0)
      {
        if ((samples_per_pixel == 4) && (photometric == PHOTOMETRIC_RGB))
          image->alpha_trait=BlendPixelTrait;
      }
    else
      {
        for (i=0; i < extra_samples; i++)
        {
          if (sample_info[i] == EXTRASAMPLE_ASSOCALPHA)
            {
              image->alpha_trait=BlendPixelTrait;
              SetQuantumAlphaType(quantum_info,AssociatedQuantumAlpha);
              (void) SetImageProperty(image,"tiff:alpha","associated",
                exception);
              break;
            }
          else
            if (sample_info[i] == EXTRASAMPLE_UNASSALPHA)
              {
                image->alpha_trait=BlendPixelTrait;
                SetQuantumAlphaType(quantum_info,DisassociatedQuantumAlpha);
                (void) SetImageProperty(image,"tiff:alpha","unassociated",
                  exception);
                break;
              }
        }
        if ((image->alpha_trait == UndefinedPixelTrait) && (extra_samples >= 1))
          {
            const char
              *option;

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
    number_pixels=MagickMax((MagickSizeType) image->columns*samples_per_pixel*
      pow(2.0,ceil(log(bits_per_sample)/log(2.0))),image->columns*
      rows_per_strip);
