// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 30/34



        /*
          TIFF has a matte channel.
        */
        (void) memset(sample_info,0,sizeof(sample_info));
        sample_info[0]=EXTRASAMPLE_UNSPECIFIED;
        if (image->alpha_trait != UndefinedPixelTrait)
          sample_info[0]=EXTRASAMPLE_UNASSALPHA;
        option=GetImageOption(image_info,"tiff:alpha");
        if (option != (const char *) NULL)
          {
            if (LocaleCompare(option,"associated") == 0)
              sample_info[0]=EXTRASAMPLE_ASSOCALPHA;
            else
              if (LocaleCompare(option,"unspecified") == 0)
                sample_info[0]=EXTRASAMPLE_UNSPECIFIED;
          }
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLESPERPIXEL,
          &samples_per_pixel,sans);
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,samples_per_pixel+
          extra_samples);
        (void) TIFFSetField(tiff,TIFFTAG_EXTRASAMPLES,(uint16) extra_samples,
          &sample_info);
        if (sample_info[0] == EXTRASAMPLE_ASSOCALPHA)
          SetQuantumAlphaType(quantum_info,AssociatedQuantumAlpha);
      }
    (void) TIFFSetField(tiff,TIFFTAG_PHOTOMETRIC,photometric);
    switch (quantum_info->format)
    {
      case FloatingPointQuantumFormat:
      {
        double
          max = 1.0,
          min = 0.0;

        (void) TIFFSetField(tiff,TIFFTAG_SAMPLEFORMAT,SAMPLEFORMAT_IEEEFP);
        (void) GetImageRange(image,&min,&max,exception);
        (void) TIFFSetField(tiff,TIFFTAG_SMINSAMPLEVALUE,min);
        (void) TIFFSetField(tiff,TIFFTAG_SMAXSAMPLEVALUE,max);
        break;
      }
      case SignedQuantumFormat:
      {
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLEFORMAT,SAMPLEFORMAT_INT);
        break;
      }
      case UnsignedQuantumFormat:
      {
        (void) TIFFSetField(tiff,TIFFTAG_SAMPLEFORMAT,SAMPLEFORMAT_UINT);
        break;
      }
      default:
        break;
    }
    (void) TIFFSetField(tiff,TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG);
    if ((photometric == PHOTOMETRIC_RGB) &&
        ((image_info->interlace == PlaneInterlace) ||
         (image_info->interlace == PartitionInterlace)))
      (void) TIFFSetField(tiff,TIFFTAG_PLANARCONFIG,PLANARCONFIG_SEPARATE);
    predictor=0;
    switch (compress_tag)
    {
      case COMPRESSION_JPEG:
      {
#if defined(JPEG_SUPPORT)
        if (image_info->quality != UndefinedCompressionQuality)
          (void) TIFFSetField(tiff,TIFFTAG_JPEGQUALITY,image_info->quality);
        (void) TIFFSetField(tiff,TIFFTAG_JPEGCOLORMODE,JPEGCOLORMODE_RAW);
        if (IssRGBCompatibleColorspace(image->colorspace) != MagickFalse)
          (void) TIFFSetField(tiff,TIFFTAG_JPEGCOLORMODE,JPEGCOLORMODE_RGB);
        if (IsYCbCrCompatibleColorspace(image->colorspace) != MagickFalse)
          {
            const char
              *sampling_factor,
              *value;

            GeometryInfo
              geometry_info;

            MagickStatusType
              flags;

            sampling_factor=(const char *) NULL;
            value=GetImageOption(image_info,"jpeg:sampling-factor");
            if (value == (char *) NULL)
              value=GetImageProperty(image,"jpeg:sampling-factor",exception);
            if (value != (char *) NULL)
              {
                sampling_factor=value;
                if (image->debug != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "  Input sampling-factors=%s",sampling_factor);
              }
            if (image_info->sampling_factor != (char *) NULL)
              sampling_factor=image_info->sampling_factor;
            if (sampling_factor != (const char *) NULL)
              {
                flags=ParseGeometry(sampling_factor,&geometry_info);
                if ((flags & SigmaValue) == 0)
                  geometry_info.sigma=geometry_info.rho;
                /*
                  To do: write pixel data in YCBCR subsampled format.

                  (void) TIFFSetField(tiff,TIFFTAG_YCBCRSUBSAMPLING,(uint16)
                    geometry_info.rho,(uint16) geometry_info.sigma);
                */
              }
            }
        (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_BITSPERSAMPLE,
          &bits_per_sample,sans);
        if (bits_per_sample == 12)
          (void) TIFFSetField(tiff,TIFFTAG_JPEGTABLESMODE,JPEGTABLESMODE_QUANT);
        option=GetImageOption(image_info,"tiff:jpeg-tables-mode");
        if (option != (char *) NULL)
          {
            int
              jpeg_tables_mode;