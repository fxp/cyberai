// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/heic.c
// Segment 5/17



          mirror=heif_item_get_property_transform_mirror(heif_context,item_id,
            transforms[0]);
          rotation_ccw=heif_item_get_property_transform_rotation_ccw(
            heif_context,item_id,transforms[0]);
          switch(mirror)
          {
            case heif_transform_mirror_direction_horizontal:
            {
              if (rotation_ccw == 0)
                image->orientation=TopRightOrientation;
              else if (rotation_ccw == 270)
                image->orientation=LeftTopOrientation;
              break;
            }
            case heif_transform_mirror_direction_vertical:
            {
              if (rotation_ccw == 0)
                image->orientation=BottomLeftOrientation;
              else if (rotation_ccw == 270)
                image->orientation=RightBottomOrientation;
              break;
            }
            case heif_transform_mirror_direction_invalid:
            {
              if (rotation_ccw == 0)
                image->orientation=TopLeftOrientation;
              else if (rotation_ccw == 90)
                image->orientation=LeftBottomOrientation;
              else if (rotation_ccw == 180)
                image->orientation=BottomRightOrientation;
              else if (rotation_ccw == 270)
                image->orientation=RightTopOrientation;
              break;
            }
          }
        }
#endif
      image->columns=(size_t) heif_image_handle_get_ispe_width(image_handle);
      image->rows=(size_t) heif_image_handle_get_ispe_height(image_handle);
    }
  if (ReadHEICColorProfile(image,image_handle,exception) == MagickFalse)
    return(MagickFalse);
  if (ReadHEICExifProfile(image,image_handle,exception) == MagickFalse)
    return(MagickFalse);
  if (ReadHEICXMPProfile(image,image_handle,exception) == MagickFalse)
    return(MagickFalse);
  if (image_info->ping != MagickFalse)
    return(MagickTrue);
  if (HEICSkipImage(image_info,image) != MagickFalse)
    return(MagickTrue);
  status=SetImageExtent(image,image->columns,image->rows,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  decode_options=heif_decoding_options_alloc();
#if LIBHEIF_NUMERIC_VERSION >= HEIC_COMPUTE_NUMERIC_VERSION(1,16,0)
  {
    const char
      *option;

    option=GetImageOption(image_info,"heic:chroma-upsampling");
    if (option != (char *) NULL)
      {
        if (LocaleCompare(option,"nearest-neighbor") == 0)
          {
            decode_options->color_conversion_options.
              only_use_preferred_chroma_algorithm=1;
            decode_options->color_conversion_options.
              preferred_chroma_upsampling_algorithm=
              heif_chroma_upsampling_nearest_neighbor;
          }
        else if (LocaleCompare(option,"bilinear") == 0)
          {
            decode_options->color_conversion_options.
              only_use_preferred_chroma_algorithm=1;
            decode_options->color_conversion_options.
              preferred_chroma_upsampling_algorithm=
              heif_chroma_upsampling_bilinear;
          }
      }
    }
#endif
  if (preserve_orientation != MagickFalse)
    decode_options->ignore_transformations=1;
  if (image->alpha_trait != UndefinedPixelTrait)
    {
      chroma=heif_chroma_interleaved_RGBA;
      if (image->depth > 8)
        chroma=heif_chroma_interleaved_RRGGBBAA_LE;
    }
  else
    {
      chroma=heif_chroma_interleaved_RGB;
      if (image->depth > 8)
        chroma=heif_chroma_interleaved_RRGGBB_LE;
    }
  error=heif_decode_image(image_handle,&heif_image,heif_colorspace_RGB,chroma,
    decode_options);
  heif_decoding_options_free(decode_options);
  if (IsHEIFSuccess(image,&error,exception) == MagickFalse)
    return(MagickFalse);
  channel=heif_channel_interleaved;
  image->columns=(size_t) heif_image_get_width(heif_image,channel);
  image->rows=(size_t) heif_image_get_height(heif_image,channel);
  status=SetImageExtent(image,image->columns,image->rows,exception);
  if (status == MagickFalse)
    {
      heif_image_release(heif_image);
      return(MagickFalse);
    }
  pixels=heif_image_get_plane_readonly(heif_image,channel,&stride);
  if (pixels == (const uint8_t *) NULL)
    {
      heif_image_release(heif_image);
      return(MagickFalse);
    }
  shift=(int) (16-image->depth);
  if (image->depth <= 8)
    for (y=0; y < (ssize_t) image->rows; y++)
    {
      Quantum
        *q;

      ssize_t
        x;