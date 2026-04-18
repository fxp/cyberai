// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jp2.c
// Segment 4/11



  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Initialize JP2 codec.
  */
  if (ReadBlob(image,sizeof(magick),magick) != sizeof(magick))
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  (void) SeekBlob(image,SEEK_SET,0);
  if (LocaleCompare(image_info->magick,"JPT") == 0)
    jp2_codec=opj_create_decompress(OPJ_CODEC_JPT);
  else
    if (IsJ2K(magick,sizeof(magick)) != MagickFalse)
      jp2_codec=opj_create_decompress(OPJ_CODEC_J2K);
    else
      if (IsJP2(magick,sizeof(magick)) != MagickFalse)
        jp2_codec=opj_create_decompress(OPJ_CODEC_JP2);
      else
        ThrowReaderException(DelegateError,"UnableToManageJP2Stream");
  opj_set_warning_handler(jp2_codec,JP2WarningHandler,exception);
  opj_set_error_handler(jp2_codec,JP2ErrorHandler,exception);
  opj_set_default_decoder_parameters(&parameters);
  option=GetImageOption(image_info,"jp2:reduce-factor");
  if (option != (const char *) NULL)
    parameters.cp_reduce=(unsigned int) StringToInteger(option);
  option=GetImageOption(image_info,"jp2:quality-layers");
  if (option != (const char *) NULL)
    parameters.cp_layer=(unsigned int) StringToInteger(option);
  if (opj_setup_decoder(jp2_codec,&parameters) == 0)
    {
      opj_destroy_codec(jp2_codec);
      ThrowReaderException(DelegateError,"UnableToManageJP2Stream");
    }
  jp2_stream=opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE,1);
  opj_stream_set_read_function(jp2_stream,JP2ReadHandler);
  opj_stream_set_write_function(jp2_stream,JP2WriteHandler);
  opj_stream_set_seek_function(jp2_stream,JP2SeekHandler);
  opj_stream_set_skip_function(jp2_stream,JP2SkipHandler);
  opj_stream_set_user_data(jp2_stream,image,NULL);
  opj_stream_set_user_data_length(jp2_stream,GetBlobSize(image));
  jp2_image=(opj_image_t *) NULL;
  if (opj_read_header(jp2_stream,jp2_codec,&jp2_image) == 0)
    {
      opj_stream_destroy(jp2_stream);
      opj_destroy_codec(jp2_codec);
      ThrowReaderException(DelegateError,"UnableToDecodeImageFile");
    }
  jp2_codestream_info=opj_get_cstr_info(jp2_codec);
  if (AcquireMagickResource(ListLengthResource,(MagickSizeType)
       jp2_codestream_info->m_default_tile_info.numlayers) == MagickFalse)
    {
      opj_destroy_cstr_info(&jp2_codestream_info);
      opj_stream_destroy(jp2_stream);
      opj_destroy_codec(jp2_codec);
      opj_image_destroy(jp2_image);
      ThrowReaderException(ResourceLimitError,"ListLengthExceedsLimit");
    }
  opj_destroy_cstr_info(&jp2_codestream_info);
  jp2_status=OPJ_TRUE;
  if ((AcquireMagickResource(WidthResource,(size_t) jp2_image->comps[0].w) == MagickFalse) ||
      (AcquireMagickResource(WidthResource,(size_t) jp2_image->x1) == MagickFalse) ||
      (AcquireMagickResource(HeightResource,(size_t) jp2_image->comps[0].h) == MagickFalse) ||
      (AcquireMagickResource(HeightResource,(size_t) jp2_image->y1) == MagickFalse))
    {
      opj_stream_destroy(jp2_stream);
      opj_destroy_codec(jp2_codec);
      opj_image_destroy(jp2_image);
      ThrowReaderException(DelegateError,"UnableToDecodeImageFile");
    }
  if (image->ping == MagickFalse)
    {
      if ((image->columns != 0) && (image->rows != 0))
        /*
          Extract an area from the image.
        */
        jp2_status=opj_set_decode_area(jp2_codec,jp2_image,
          (OPJ_INT32) image->extract_info.x,(OPJ_INT32) image->extract_info.y,
          (OPJ_INT32) (image->extract_info.x+(ssize_t) image->columns),
          (OPJ_INT32) (image->extract_info.y+(ssize_t) image->rows));
      else
        jp2_status=opj_set_decode_area(jp2_codec,jp2_image,0,0,
          (int) jp2_image->comps[0].w,(int) jp2_image->comps[0].h);
      if (jp2_status == OPJ_FALSE)
        {
          opj_stream_destroy(jp2_stream);
          opj_destroy_codec(jp2_codec);
          opj_image_destroy(jp2_image);
          ThrowReaderException(DelegateError,"UnableToDecodeImageFile");
        }
    }
  if ((image_info->number_scenes != 0) && (image_info->scene != 0))
    jp2_status=opj_get_decoded_tile(jp2_codec,jp2_stream,jp2_image,
      (unsigned int) image_info->scene-1);
  else
    if (image->ping == MagickFalse)
      {
        jp2_status=opj_decode(jp2_codec,jp2_stream,jp2_image);
        if (jp2_status != OPJ_FALSE)
          jp2_status=opj_end_decompress(jp2_codec,jp2_stream);
      }
  if (jp2_status == OPJ_FALSE)
    {
      opj_stream_destroy(jp2_stream);
      opj_destroy_codec(jp2_codec);
      opj_image_destroy(jp2_image);