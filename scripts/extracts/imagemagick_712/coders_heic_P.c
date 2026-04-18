// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 16/17



        SetGeometryInfo(&cicp);
        nclx_profile=heif_nclx_color_profile_alloc();
        if (nclx_profile == (struct heif_color_profile_nclx *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        cicp.rho=(double) nclx_profile->color_primaries;
        cicp.sigma=(double) nclx_profile->transfer_characteristics;
        cicp.xi=(double) nclx_profile->matrix_coefficients;
        cicp.psi=(double) nclx_profile->full_range_flag;
        (void) ParseGeometry(option,&cicp);
        heif_nclx_color_profile_set_color_primaries(nclx_profile,
          (uint16_t) cicp.rho);
        heif_nclx_color_profile_set_transfer_characteristics(nclx_profile,
          (uint16_t) cicp.sigma);
        heif_nclx_color_profile_set_matrix_coefficients(nclx_profile,
          (uint16_t) cicp.xi);
        nclx_profile->full_range_flag=(uint8_t) cicp.psi; 
        heif_image_set_nclx_color_profile(heif_image,nclx_profile);
        heif_nclx_color_profile_free(nclx_profile);
      }
#endif
    profile=GetImageProfile(image,"icc");
    if (profile != (StringInfo *) NULL)
      (void) heif_image_set_raw_color_profile(heif_image,"prof",
        GetStringInfoDatum(profile),GetStringInfoLength(profile));
    if (colorspace == heif_colorspace_YCbCr)
      status=WriteHEICImageYCbCr(image,heif_image,exception);
    else
      if (image->depth > 8)
        status=WriteHEICImageRRGGBBAA(image,heif_image,exception);
      else
        status=WriteHEICImageRGBA(image,heif_image,exception);
    if (status == MagickFalse)
      break;
    /*
      Encode HEIC image.
    */
    if (lossless != MagickFalse)
      error=heif_encoder_set_lossless(heif_encoder,1);
    else if (image_info->quality != UndefinedCompressionQuality)
      error=heif_encoder_set_lossy_quality(heif_encoder,(int)
        image_info->quality);
    status=IsHEIFSuccess(image,&error,exception);
    if (status == MagickFalse)
      break;
    option=GetImageOption(image_info,"heic:speed");
    if (option != (char *) NULL)
      {
        error=heif_encoder_set_parameter(heif_encoder,"speed",option);
        status=IsHEIFSuccess(image,&error,exception);
        if (status == MagickFalse)
          break;
      }
    option=GetImageOption(image_info,"heic:chroma");
    if (option != (char *) NULL)
      {
        error=heif_encoder_set_parameter(heif_encoder,"chroma",option);
        status=IsHEIFSuccess(image,&error,exception);
        if (status == MagickFalse)
          break;
      }
    options=heif_encoding_options_alloc();
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,16,0)
    option=GetImageOption(image_info,"heic:chroma-downsampling");
    if (option != (char *) NULL)
      {
        if (LocaleCompare(option,"nearest-neighbor") == 0)
          {
            options->color_conversion_options.
              only_use_preferred_chroma_algorithm=1;
            options->color_conversion_options.
              preferred_chroma_downsampling_algorithm=
              heif_chroma_downsampling_nearest_neighbor;
          }
        else if (LocaleCompare(option,"average") == 0)
          {
            options->color_conversion_options.
              only_use_preferred_chroma_algorithm=1;
            options->color_conversion_options.
              preferred_chroma_downsampling_algorithm=
              heif_chroma_downsampling_average;
          }
        else if (LocaleCompare(option,"sharp-yuv") == 0)
          {
            options->color_conversion_options.
              only_use_preferred_chroma_algorithm=1;
            options->color_conversion_options.
              preferred_chroma_downsampling_algorithm=
              heif_chroma_downsampling_sharp_yuv;
          }
      }
#endif
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,14,0)
    if (image->orientation != UndefinedOrientation)
      options->image_orientation=(enum heif_orientation) image->orientation;
#endif
    error=heif_context_encode_image(heif_context,heif_image,heif_encoder,
      options,(struct heif_image_handle **) NULL);
    heif_encoding_options_free(options);
    status=IsHEIFSuccess(image,&error,exception);
    if (status == MagickFalse)
      break;
    if (image->profiles != (void *) NULL)
      WriteProfile(heif_context,image,exception);
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    status=SetImageProgress(image,SaveImagesTag,scene,
      GetImageListLength(image));
    if (status == MagickFalse)
      break;
    heif_encoder_release(heif_encoder);
    heif_encoder=(struct heif_encoder*) NULL;
    heif_image_release(heif_image);
    heif_image=(struct heif_image*) NULL;
    scene++;
  } while (image_info->adjoin != MagickFalse);
  if (status != MagickFalse)
    {
      struct heif_writer
        writer;