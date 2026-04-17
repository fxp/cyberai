/* ===== EXTRACT: tiff.c ===== */
/* Function: ReadTIFFImage (part C) */
/* Lines: 1435–1532 */

    if (TIFFIsBigEndian(tiff) == 0)
      {
        (void) SetImageProperty(image,"tiff:endian","lsb",exception);
        image->endian=LSBEndian;
      }
    else
      {
        (void) SetImageProperty(image,"tiff:endian","msb",exception);
        image->endian=MSBEndian;
      }
    if ((photometric == PHOTOMETRIC_MINISBLACK) ||
        (photometric == PHOTOMETRIC_MINISWHITE))
      (void) SetImageColorspace(image,GRAYColorspace,exception);
    if (photometric == PHOTOMETRIC_SEPARATED)
      (void) SetImageColorspace(image,CMYKColorspace,exception);
    if (photometric == PHOTOMETRIC_CIELAB)
      (void) SetImageColorspace(image,LabColorspace,exception);
    if ((photometric == PHOTOMETRIC_YCBCR) &&
        (compress_tag != COMPRESSION_OJPEG) &&
        (compress_tag != COMPRESSION_JPEG))
      (void) SetImageColorspace(image,YCbCrColorspace,exception);
    TIFFGetProfiles(tiff,image,exception);
    status=TIFFGetProperties(tiff,image,exception);
    if (status == MagickFalse)
      {
        TIFFClose(tiff);
        return(DestroyImageList(image));
      }
    TIFFGetEXIFProperties(tiff,image,image_info,exception);
    TIFFGetGPSProperties(tiff,image,image_info,exception);
    if ((TIFFGetFieldDefaulted(tiff,TIFFTAG_XRESOLUTION,&x_resolution,sans) == 1) &&
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_YRESOLUTION,&y_resolution,sans) == 1))
      {
        image->resolution.x=x_resolution;
        image->resolution.y=y_resolution;
      }
    if (TIFFGetFieldDefaulted(tiff,TIFFTAG_RESOLUTIONUNIT,&units,sans,sans) == 1)
      {
        if (units == RESUNIT_INCH)
          image->units=PixelsPerInchResolution;
        if (units == RESUNIT_CENTIMETER)
          image->units=PixelsPerCentimeterResolution;
      }
    if ((TIFFGetFieldDefaulted(tiff,TIFFTAG_XPOSITION,&x_position,sans) == 1) &&
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_YPOSITION,&y_position,sans) == 1))
      {
        image->page.x=CastDoubleToLong(ceil((double) x_position*
          image->resolution.x-0.5));
        image->page.y=CastDoubleToLong(ceil((double) y_position*
          image->resolution.y-0.5));
      }
    if (TIFFGetFieldDefaulted(tiff,TIFFTAG_ORIENTATION,&orientation,sans) == 1)
      image->orientation=(OrientationType) orientation;
    if (TIFFGetField(tiff,TIFFTAG_WHITEPOINT,&chromaticity) == 1)
      {
        if ((chromaticity != (float *) NULL) && (*chromaticity != 0.0f))
          {
            image->chromaticity.white_point.x=chromaticity[0];
            image->chromaticity.white_point.y=chromaticity[1];
          }
      }
    if (TIFFGetField(tiff,TIFFTAG_PRIMARYCHROMATICITIES,&chromaticity) == 1)
      {
        if ((chromaticity != (float *) NULL) && (*chromaticity != 0.0f))
          {
            image->chromaticity.red_primary.x=chromaticity[0];
            image->chromaticity.red_primary.y=chromaticity[1];
            image->chromaticity.green_primary.x=chromaticity[2];
            image->chromaticity.green_primary.y=chromaticity[3];
            image->chromaticity.blue_primary.x=chromaticity[4];
            image->chromaticity.blue_primary.y=chromaticity[5];
          }
      }
    if ((compress_tag != COMPRESSION_NONE) &&
        (TIFFIsCODECConfigured(compress_tag) == 0))
      {
        TIFFClose(tiff);
        ThrowReaderException(CoderError,"CompressNotSupported");
      }
    switch (compress_tag)
    {
      case COMPRESSION_NONE: image->compression=NoCompression; break;
      case COMPRESSION_CCITTFAX3: image->compression=FaxCompression; break;
      case COMPRESSION_CCITTFAX4: image->compression=Group4Compression; break;
      case COMPRESSION_JPEG:
      {
         image->compression=JPEGCompression;
#if defined(JPEG_SUPPORT)
         {
           char
             sampling_factor[MagickPathExtent];

           uint16
             horizontal,
             vertical;

           tiff_status=TIFFGetField(tiff,TIFFTAG_YCBCRSUBSAMPLING,&horizontal,
             &vertical);
