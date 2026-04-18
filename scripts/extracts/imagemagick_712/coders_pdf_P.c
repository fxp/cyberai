// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 16/30



    icc_profile=GetCompatibleColorProfile(image);
    compression=image->compression;
    if (image_info->compression != UndefinedCompression)
      compression=image_info->compression;
    switch (compression)
    {
      case FaxCompression:
      case Group4Compression:
      {
        if ((SetImageMonochrome(image,exception) == MagickFalse) ||
            (image->alpha_trait != UndefinedPixelTrait))
          compression=RLECompression;
        break;
      }
#if !defined(MAGICKCORE_JPEG_DELEGATE)
      case JPEGCompression:
      {
        compression=RLECompression;
        (void) ThrowMagickException(exception,GetMagickModule(),
          MissingDelegateError,"DelegateLibrarySupportNotBuiltIn","`%s' (JPEG)",
          image->filename);
        break;
      }
#endif
#if !defined(MAGICKCORE_LIBOPENJP2_DELEGATE)
      case JPEG2000Compression:
      {
        compression=RLECompression;
        (void) ThrowMagickException(exception,GetMagickModule(),
          MissingDelegateError,"DelegateLibrarySupportNotBuiltIn","`%s' (JP2)",
          image->filename);
        break;
      }
#endif
#if !defined(MAGICKCORE_ZLIB_DELEGATE)
      case ZipCompression:
      {
        compression=RLECompression;
        (void) ThrowMagickException(exception,GetMagickModule(),
          MissingDelegateError,"DelegateLibrarySupportNotBuiltIn","`%s' (ZLIB)",
          image->filename);
        break;
      }
#endif
      case LZWCompression:
      {
        if (is_pdfa != MagickFalse)
          compression=RLECompression;  /* LZW compression is forbidden */
        break;
      }
      case NoCompression:
      {
        if (is_pdfa != MagickFalse)
          compression=RLECompression; /* ASCII 85 compression is forbidden */
        break;
      }
      default:
        break;
    }
    if (compression == JPEG2000Compression)
      if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
      (void) TransformImageColorspace(image,sRGBColorspace,exception);
    /*
      Scale relative to dots-per-inch.
    */
    delta.x=DefaultResolution;
    delta.y=DefaultResolution;
    resolution.x=image->resolution.x;
    resolution.y=image->resolution.y;
    if ((resolution.x == 0.0) || (resolution.y == 0.0))
      {
        flags=ParseGeometry(PSDensityGeometry,&geometry_info);
        if ((flags & RhoValue) != 0)
          resolution.x=geometry_info.rho;
        resolution.y=resolution.x;
        if ((flags & SigmaValue) != 0)
          resolution.y=geometry_info.sigma;
      }
    if (image_info->density != (char *) NULL)
      {
        flags=ParseGeometry(image_info->density,&geometry_info);
        if ((flags & RhoValue) != 0)
          resolution.x=geometry_info.rho;
        resolution.y=resolution.x;
        if ((flags & SigmaValue) != 0)
          resolution.y=geometry_info.sigma;
      }
    if (image->units == PixelsPerCentimeterResolution)
      {
        resolution.x=(double) ((size_t) (100.0*2.54*resolution.x+0.5)/100.0);
        resolution.y=(double) ((size_t) (100.0*2.54*resolution.y+0.5)/100.0);
      }
    SetGeometry(image,&geometry);
    (void) FormatLocaleString(temp,MagickPathExtent,"%.20gx%.20g",
      (double) image->columns,(double) image->rows);
    if (image_info->page != (char *) NULL)
      (void) CopyMagickString(temp,image_info->page,MagickPathExtent);
    else
      if ((image->page.width != 0) && (image->page.height != 0))
        (void) FormatLocaleString(temp,MagickPathExtent,
          "%.20gx%.20g%+.20g%+.20g",(double) image->page.width,(double)
          image->page.height,(double) image->page.x,(double) image->page.y);
      else
        if ((image->gravity != UndefinedGravity) &&
            (LocaleCompare(image_info->magick,"PDF") == 0))
          (void) CopyMagickString(temp,PSPageGeometry,
            MagickPathExtent);
    (void) ConcatenateMagickString(temp,">",MagickPathExtent);
    (void) ParseMetaGeometry(temp,&geometry.x,&geometry.y,
      &geometry.width,&geometry.height);
    scale.x=(double) (geometry.width*delta.x)/resolution.x;
    geometry.width=CastDoubleToSizeT(scale.x+0.5);
    scale.y=(double) (geometry.height*delta.y)/resolution.y;
    geometry.height=CastDoubleToSizeT(scale.y+0.5);
    (void) ParseAbsoluteGeometry(temp,&media_info);
    (void) ParseGravityGeometry(image,temp,&page_info,exception);
    if (image->gravity != UndefinedGravity)
      {
        geometry.x=(-page_info.x);
        geometry.y=(ssize_t) media_info.height+page_info.y-(ssize_t)
          image->rows;
      }
    pointsize=12.0;
    if (image_info->pointsize != 0.0)
      pointsize=image_info->pointsize;
    text_size=0;
    value=GetImageProperty(image,"label",exception);
    if (value != (const char *) NULL)
      text_size=(size_t) (MultilineCensus(value)*pointsize+12);
    (void) text_size;
    /*
      Write Page object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g 0 obj\n",(double)
      object);
    (void) WriteBlobString(image