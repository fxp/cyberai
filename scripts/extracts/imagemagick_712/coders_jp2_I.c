// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jp2.c
// Segment 9/11



  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(status);
  /*
    Initialize JPEG 2000 API.
  */
  parameters=(opj_cparameters_t *) AcquireMagickMemory(sizeof(*parameters));
  if (parameters == (opj_cparameters_t *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  opj_set_default_encoder_parameters(parameters);
  option=GetImageOption(image_info,"jp2:number-resolutions");
  if (option != (const char *) NULL)
    parameters->numresolution=StringToInteger(option);
  else
    parameters->numresolution=CalculateNumResolutions(image->columns,
      image->rows);
  parameters->tcp_numlayers=1;
  parameters->tcp_rates[0]=0;  /* lossless */
  parameters->cp_disto_alloc=OPJ_TRUE;
  if ((image_info->quality != 0) && (image_info->quality != 100))
    {
      parameters->tcp_distoratio[0]=(float) image_info->quality;
      parameters->cp_fixed_quality=OPJ_TRUE;
      parameters->cp_disto_alloc=OPJ_FALSE;
    }
  if (image_info->extract != (char *) NULL)
    {
      int
        flags;

      RectangleInfo
        geometry;

      /*
        Set tile size.
      */
      (void) memset(&geometry,0,sizeof(geometry));
      flags=(int) ParseAbsoluteGeometry(image_info->extract,&geometry);
      parameters->cp_tdx=(int) geometry.width;
      parameters->cp_tdy=(int) geometry.width;
      if ((flags & HeightValue) != 0)
        parameters->cp_tdy=(int) geometry.height;
      if ((flags & XValue) != 0)
        parameters->cp_tx0=(int) geometry.x;
      if ((flags & YValue) != 0)
        parameters->cp_ty0=(int) geometry.y;
      parameters->tile_size_on=OPJ_TRUE;
      parameters->numresolution=CalculateNumResolutions((size_t)
        parameters->cp_tdx,(size_t) parameters->cp_tdy);
    }
  option=GetImageOption(image_info,"jp2:quality");
  if (option != (const char *) NULL)
    {
      const char
        *p;

      /*
        Set quality PSNR.
      */
      p=option;
      for (i=0; MagickSscanf(p,"%f",&parameters->tcp_distoratio[i]) == 1; i++)
      {
        if (i > 100)
          break;
        while ((*p != '\0') && (*p != ','))
          p++;
        if (*p == '\0')
          break;
        p++;
      }
      parameters->tcp_numlayers=(int) (i+1);
      parameters->cp_fixed_quality=OPJ_TRUE;
      parameters->cp_disto_alloc=OPJ_FALSE;
    }
  option=GetImageOption(image_info,"jp2:progression-order");
  if (option != (const char *) NULL)
    {
      if (LocaleCompare(option,"LRCP") == 0)
        parameters->prog_order=OPJ_LRCP;
      if (LocaleCompare(option,"RLCP") == 0)
        parameters->prog_order=OPJ_RLCP;
      if (LocaleCompare(option,"RPCL") == 0)
        parameters->prog_order=OPJ_RPCL;
      if (LocaleCompare(option,"PCRL") == 0)
        parameters->prog_order=OPJ_PCRL;
      if (LocaleCompare(option,"CPRL") == 0)
        parameters->prog_order=OPJ_CPRL;
    }
  option=GetImageOption(image_info,"jp2:rate");
  if (option != (const char *) NULL)
    {
      const char
        *p;

      /*
        Set compression rate.
      */
      p=option;
      for (i=0; MagickSscanf(p,"%f",&parameters->tcp_rates[i]) == 1; i++)
      {
        if (i >= 100)
          break;
        while ((*p != '\0') && (*p != ','))
          p++;
        if (*p == '\0')
          break;
        p++;
      }
      parameters->tcp_numlayers=(int) (i+1);
      parameters->cp_disto_alloc=OPJ_TRUE;
    }
  if (image_info->sampling_factor != (char *) NULL)
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;