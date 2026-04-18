// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jp2.c
// Segment 10/11



      flags=ParseGeometry(image_info->sampling_factor,&geometry_info);
      if ((flags & RhoValue) != 0)
        parameters->subsampling_dx=(int) MagickMax(
          geometry_info.rho,1.0);
      parameters->subsampling_dy=parameters->subsampling_dx;
      if ((flags & SigmaValue) != 0)
        parameters->subsampling_dy=(int) MagickMax(
          geometry_info.sigma,1.0);
    }   
  property=GetImageProperty(image,"comment",exception);
  if (property != (const char *) NULL)
    parameters->cp_comment=(char *) property;
  channels=3;
  jp2_colorspace=OPJ_CLRSPC_SRGB;
  if (image->colorspace == YUVColorspace)
    jp2_colorspace=OPJ_CLRSPC_SYCC;
  else
    {
      if (IsGrayColorspace(image->colorspace) != MagickFalse)
        {
          channels=1;
          jp2_colorspace=OPJ_CLRSPC_GRAY;
        }
      else
        if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
          (void) TransformImageColorspace(image,sRGBColorspace,exception);
      if (image->alpha_trait != UndefinedPixelTrait)
        channels++;
    }
  parameters->tcp_mct=channels == 3 ? 1 : 0;
  memset(jp2_info,0,sizeof(jp2_info));
  for (i=0; i < (ssize_t) channels; i++)
  {
    jp2_info[i].prec=(OPJ_UINT32) image->depth;
    if ((image->depth == 1) &&
        ((LocaleCompare(image_info->magick,"JPT") == 0) ||
         (LocaleCompare(image_info->magick,"JP2") == 0)))
      jp2_info[i].prec++;  /* OpenJPEG returns exception for depth @ 1 */
    jp2_info[i].sgnd=0;
    jp2_info[i].dx=(unsigned int) parameters->subsampling_dx;
    jp2_info[i].dy=(unsigned int) parameters->subsampling_dy;
    jp2_info[i].w=(OPJ_UINT32) image->columns;
    jp2_info[i].h=(OPJ_UINT32) image->rows;
  }
  jp2_image=opj_image_create((OPJ_UINT32) channels,jp2_info,jp2_colorspace);
  if (jp2_image == (opj_image_t *) NULL)
    {
      parameters=(opj_cparameters_t *) RelinquishMagickMemory(parameters);
      ThrowWriterException(DelegateError,"UnableToEncodeImageFile");
    }
  jp2_image->x0=(unsigned int) parameters->image_offset_x0;
  jp2_image->y0=(unsigned int) parameters->image_offset_y0;
  jp2_image->x1=(unsigned int) (2*parameters->image_offset_x0+
    ((ssize_t) image->columns-1)*parameters->subsampling_dx+1);
  jp2_image->y1=(unsigned int) (2*parameters->image_offset_y0+
    ((ssize_t) image->rows-1)*parameters->subsampling_dx+1);
  if ((image->depth == 12) &&
      ((image->columns == 2048) || (image->rows == 1080) ||
       (image->columns == 4096) || (image->rows == 2160)))
    CinemaProfileCompliance(jp2_image,parameters);
  if (channels == 4)
    jp2_image->comps[3].alpha=1;
  else
    if ((channels == 2) && (jp2_colorspace == OPJ_CLRSPC_GRAY))
      jp2_image->comps[1].alpha=1;
  /*
    Convert to JP2 pixels.
  */
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *p;

    ssize_t
      x;

    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      for (i=0; i < (ssize_t) channels; i++)
      {
        double
          scale;

        int
          *q;