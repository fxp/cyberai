// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jp2.c
// Segment 5/11


      ThrowReaderException(DelegateError,"UnableToDecodeImageFile");
    }
  if (jp2_image->numcomps >= MaxPixelChannels)
    {
      opj_stream_destroy(jp2_stream);
      opj_destroy_codec(jp2_codec);
      opj_image_destroy(jp2_image);
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    }
  for (i=0; i < (ssize_t) jp2_image->numcomps; i++)
  {
    if ((jp2_image->comps[i].dx == 0) || (jp2_image->comps[i].dy == 0) ||
        (jp2_image->comps[0].prec != jp2_image->comps[i].prec) ||
        (jp2_image->comps[0].prec > 64) ||
        (jp2_image->comps[0].sgnd != jp2_image->comps[i].sgnd))
      {
        opj_stream_destroy(jp2_stream);
        opj_destroy_codec(jp2_codec);
        opj_image_destroy(jp2_image);
        ThrowReaderException(CoderError,"IrregularChannelGeometryNotSupported")
      }
  }
  opj_stream_destroy(jp2_stream);
  if (image->ping == MagickFalse)
    {
      for (i=0; i < (ssize_t) jp2_image->numcomps; i++)
        if (jp2_image->comps[i].data == NULL)
          {
            opj_destroy_codec(jp2_codec);
            opj_image_destroy(jp2_image);
            ThrowReaderException(CoderError,
              "IrregularChannelGeometryNotSupported")
          }
    }
  /*
    Convert JP2 image.
  */
  image->columns=(size_t) jp2_image->comps[0].w;
  image->rows=(size_t) jp2_image->comps[0].h;
  image->depth=jp2_image->comps[0].prec;
  image->compression=JPEG2000Compression;
  if (jp2_image->numcomps == 1)
    SetImageColorspace(image,GRAYColorspace,exception);
  else
    if (jp2_image->color_space == 2)
      SetImageColorspace(image,GRAYColorspace,exception);
    else
      if (jp2_image->color_space == 3)
        SetImageColorspace(image,Rec601YCbCrColorspace,exception);
  if (jp2_image->numcomps > 3)
    {
      size_t
        number_meta_channels;

      number_meta_channels=jp2_image->numcomps-3;
      if (JP2ComponentHasAlpha(image_info,jp2_image->comps[3]) != MagickFalse)
        {
          image->alpha_trait=BlendPixelTrait;
          number_meta_channels-=1;
        }
      if (number_meta_channels > 0)
        {
          status=SetPixelMetaChannels(image,(size_t) number_meta_channels,
            exception);
          if (status == MagickFalse)
            {
              opj_destroy_codec(jp2_codec);
              opj_image_destroy(jp2_image);
              return(DestroyImageList(image));
            }
        }
    }
  else if (jp2_image->numcomps == 2)
    {
      if (JP2ComponentHasAlpha(image_info,jp2_image->comps[1]) != MagickFalse)
        image->alpha_trait=BlendPixelTrait;
    }
  else if ((jp2_image->numcomps == 1) && (jp2_image->comps[0].alpha != 0))
    image->alpha_trait=BlendPixelTrait;
  if (jp2_image->icc_profile_buf != (unsigned char *) NULL)
    {
      StringInfo
        *profile;

      profile=BlobToProfileStringInfo("icc",jp2_image->icc_profile_buf,
        jp2_image->icc_profile_len,exception);
      (void) SetImageProfilePrivate(image,profile,exception);
    }
  if (image->ping != MagickFalse)
    {
      opj_destroy_codec(jp2_codec);
      opj_image_destroy(jp2_image);
      return(GetFirstImageInList(image));
    }
  status=SetImageExtent(image,image->columns,image->rows,exception);
  if (status == MagickFalse)
    {
      opj_destroy_codec(jp2_codec);
      opj_image_destroy(jp2_image);
      return(DestroyImageList(image));
    }
  memset(comps_info,0,MaxPixelChannels*sizeof(JP2CompsInfo));
  for (i=0; i < (ssize_t) jp2_image->numcomps; i++)
  {
    comps_info[i].scale=(double) QuantumRange/(double) 
      ((MagickULLConstant(1) << jp2_image->comps[i].prec)-1);
    comps_info[i].addition=(jp2_image->comps[i].sgnd ?
      MagickULLConstant(1) << (jp2_image->comps[i].prec-1) : 0);
    comps_info[i].pad=(ssize_t) image->columns % jp2_image->comps[i].dx;
  }
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    for (i=0; i < (ssize_t) jp2_image->numcomps; i++)
    {
      comps_info[i].y_index=y/jp2_image->comps[i].dy*((ssize_t) image->columns+
        comps_info[i].pad);
    }
    q=GetAuthenticPixels(image,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      for (i=0; i < (ssize_t) jp2_image->numcomps; i++)
      {
        double
          pixel;

        ssize_t
          index;