// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jp2.c
// Segment 11/11



        scale=(double) (((size_t) MagickULLConstant(1) <<
          jp2_image->comps[i].prec)-1)/(double) QuantumRange;
        q=jp2_image->comps[i].data+(ssize_t) (y*MagickSafeReciprocal(
          jp2_image->comps[i].dy)*image->columns*MagickSafeReciprocal(
          jp2_image->comps[i].dx)+x*MagickSafeReciprocal(
          jp2_image->comps[i].dx));
        switch (i)
        {
          case 0:
          {
            if (jp2_colorspace == OPJ_CLRSPC_GRAY)
              {
                *q=(int) (scale*(double) GetPixelGray(image,p));
                break;
              }
            *q=(int) (scale*(double) GetPixelRed(image,p));
            break;
          }
          case 1:
          {
            if (jp2_colorspace == OPJ_CLRSPC_GRAY)
              {
                *q=(int) (scale*(double) GetPixelAlpha(image,p));
                break;
              }
            *q=(int) (scale*(double) GetPixelGreen(image,p));
            break;
          }
          case 2:
          {
            *q=(int) (scale*(double) GetPixelBlue(image,p));
            break;
          }
          case 3:
          {
            *q=(int) (scale*(double) GetPixelAlpha(image,p));
            break;
          }
        }
      }
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
    status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
      image->rows);
    if (status == MagickFalse)
      break;
  }
  if (LocaleCompare(image_info->magick,"JPT") == 0)
    jp2_codec=opj_create_compress(OPJ_CODEC_JPT);
  else
    if (LocaleCompare(image_info->magick,"J2K") == 0)
      jp2_codec=opj_create_compress(OPJ_CODEC_J2K);
    else
      jp2_codec=opj_create_compress(OPJ_CODEC_JP2);
  opj_set_warning_handler(jp2_codec,JP2WarningHandler,exception);
  opj_set_error_handler(jp2_codec,JP2ErrorHandler,exception);
  opj_setup_encoder(jp2_codec,parameters,jp2_image);
  jp2_stream=opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE,OPJ_FALSE);
  if (jp2_stream == (opj_stream_t *) NULL)
    {
      opj_destroy_codec(jp2_codec);
      opj_image_destroy(jp2_image);
      parameters=(opj_cparameters_t *) RelinquishMagickMemory(parameters);
      ThrowWriterException(DelegateError,"UnableToEncodeImageFile");
    }
  opj_stream_set_read_function(jp2_stream,JP2ReadHandler);
  opj_stream_set_write_function(jp2_stream,JP2WriteHandler);
  opj_stream_set_seek_function(jp2_stream,JP2SeekHandler);
  opj_stream_set_skip_function(jp2_stream,JP2SkipHandler);
  opj_stream_set_user_data(jp2_stream,image,NULL);
  jp2_status=opj_start_compress(jp2_codec,jp2_image,jp2_stream);
  if ((jp2_status == 0) || (opj_encode(jp2_codec,jp2_stream) == 0) ||
      (opj_end_compress(jp2_codec,jp2_stream) == 0))
    {
      opj_stream_destroy(jp2_stream);
      opj_destroy_codec(jp2_codec);
      opj_image_destroy(jp2_image);
      parameters=(opj_cparameters_t *) RelinquishMagickMemory(parameters);
      ThrowWriterException(DelegateError,"UnableToEncodeImageFile");
    }
  /*
    Free resources.
  */
  opj_stream_destroy(jp2_stream);
  opj_destroy_codec(jp2_codec);
  opj_image_destroy(jp2_image);
  parameters=(opj_cparameters_t *) RelinquishMagickMemory(parameters);
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  return(status);
}
#endif
