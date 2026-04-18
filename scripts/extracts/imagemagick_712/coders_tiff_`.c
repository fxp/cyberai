// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 32/34



        /*
          Set image resolution.
        */
        units=RESUNIT_NONE;
        if (image->units == PixelsPerInchResolution)
          units=RESUNIT_INCH;
        if (image->units == PixelsPerCentimeterResolution)
          units=RESUNIT_CENTIMETER;
        (void) TIFFSetField(tiff,TIFFTAG_RESOLUTIONUNIT,(uint16) units);
        (void) TIFFSetField(tiff,TIFFTAG_XRESOLUTION,image->resolution.x);
        (void) TIFFSetField(tiff,TIFFTAG_YRESOLUTION,image->resolution.y);
        if ((x_position < 0) || (y_position < 0))
          {
            x_position=MagickMax(x_position,0);
            y_position=MagickMax(y_position,0);
            (void) ThrowMagickException(exception,GetMagickModule(),
              CoderWarning,"TIFF: negative image positions unsupported","%s",
              image->filename);
          }
        (void) TIFFSetField(tiff,TIFFTAG_XPOSITION,(double) x_position/
          image->resolution.x);
        (void) TIFFSetField(tiff,TIFFTAG_YPOSITION,(double) y_position/
          image->resolution.y);
      }
    if (image->chromaticity.white_point.x != 0.0)
      {
        float
          chromaticity[6];

        /*
          Set image chromaticity.
        */
        chromaticity[0]=(float) image->chromaticity.red_primary.x;
        chromaticity[1]=(float) image->chromaticity.red_primary.y;
        chromaticity[2]=(float) image->chromaticity.green_primary.x;
        chromaticity[3]=(float) image->chromaticity.green_primary.y;
        chromaticity[4]=(float) image->chromaticity.blue_primary.x;
        chromaticity[5]=(float) image->chromaticity.blue_primary.y;
        (void) TIFFSetField(tiff,TIFFTAG_PRIMARYCHROMATICITIES,chromaticity);
        chromaticity[0]=(float) image->chromaticity.white_point.x;
        chromaticity[1]=(float) image->chromaticity.white_point.y;
        (void) TIFFSetField(tiff,TIFFTAG_WHITEPOINT,chromaticity);
      }
    option=GetImageOption(image_info,"tiff:write-layers");
    if (IsStringTrue(option) != MagickFalse)
      {
        (void) TIFFWritePhotoshopLayers(image,image_info,endian_type,exception);
        adjoin=MagickFalse;
      }
    if ((LocaleCompare(image_info->magick,"PTIF") != 0) &&
        (adjoin != MagickFalse) && (number_scenes > 1))
      {
        (void) TIFFSetField(tiff,TIFFTAG_SUBFILETYPE,FILETYPE_PAGE);
        if (image->scene != 0)
          (void) TIFFSetField(tiff,TIFFTAG_PAGENUMBER,(uint16) image->scene,
            number_scenes);
      }
    if (image->orientation != UndefinedOrientation)
      (void) TIFFSetField(tiff,TIFFTAG_ORIENTATION,(uint16) image->orientation);
    else
      (void) TIFFSetField(tiff,TIFFTAG_ORIENTATION,ORIENTATION_TOPLEFT);
    TIFFSetProfiles(tiff,image);
    {
      uint16
        page,
        pages;

      page=(uint16) scene;
      pages=(uint16) number_scenes;
      if ((LocaleCompare(image_info->magick,"PTIF") != 0) &&
          (adjoin != MagickFalse) && (pages > 1))
        (void) TIFFSetField(tiff,TIFFTAG_SUBFILETYPE,FILETYPE_PAGE);
      (void) TIFFSetField(tiff,TIFFTAG_PAGENUMBER,page,pages);
    }
    (void) TIFFSetProperties(tiff,adjoin,image,exception);
    /*
      Write image scanlines.
    */
    if (GetTIFFInfo(image_info,tiff,&tiff_info) == MagickFalse)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    if (compress_tag == COMPRESSION_CCITTFAX4)
      (void) TIFFSetField(tiff,TIFFTAG_ROWSPERSTRIP,(uint32) image->rows);
    (void) SetQuantumEndian(image,quantum_info,LSBEndian);
    pixels=(unsigned char *) GetQuantumPixels(quantum_info);
    tiff_info.scanline=(unsigned char *) GetQuantumPixels(quantum_info);
    switch (photometric)
    {
      case PHOTOMETRIC_CIELAB:
      case PHOTOMETRIC_YCBCR:
      case PHOTOMETRIC_RGB:
      {
        /*
          RGB TIFF image.
        */
        switch (image_info->interlace)
        {
          case NoInterlace:
          default:
          {
            quantum_type=RGBQuantum;
            if (image->alpha_trait != UndefinedPixelTrait)
              quantum_type=RGBAQuantum;
            if (image->number_meta_channels != 0)
              {
                quantum_type=MultispectralQuantum;
                (void) SetQuantumPad(image,quantum_info,0);
              }
            status=WriteTIFFChannels(image,tiff,tiff_info,quantum_info,
              quantum_type,0,pixels,exception);
            break;
          }
          case PlaneInterlace:
          case PartitionInterlace:
          {
            tsample_t
              sample = 0;