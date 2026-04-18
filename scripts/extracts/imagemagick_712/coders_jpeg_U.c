// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 21/24



      extent_info=CloneImageInfo(image_info);
      extent_info->blob=NULL;
      jpeg_image=CloneImage(image,0,0,MagickTrue,exception);
      if (jpeg_image != (Image *) NULL)
        {
          MagickSizeType
            extent;

          size_t
            maximum,
            minimum;

          /*
            Search for compression quality that does not exceed image extent.
          */
          extent_info->quality=0;
          extent=(MagickSizeType) SiPrefixToDoubleInterval(option,100.0);
          (void) DeleteImageOption(extent_info,"jpeg:extent");
          (void) DeleteImageArtifact(jpeg_image,"jpeg:extent");
          maximum=image_info->quality;
          if (maximum < 2)
            maximum=101;
          for (minimum=2; minimum < maximum; )
          {
            (void) AcquireUniqueFilename(jpeg_image->filename);
            jpeg_image->quality=minimum+(maximum-minimum+1)/2;
            status=WriteJPEGImage(extent_info,jpeg_image,exception);
            (void) RelinquishUniqueFileResource(jpeg_image->filename);
            if (status == MagickFalse)
              break;
            if (GetBlobSize(jpeg_image) <= extent)
              minimum=jpeg_image->quality+1;
            else
              maximum=jpeg_image->quality-1;
          }
          while (minimum > 2)
          {
            (void) AcquireUniqueFilename(jpeg_image->filename);
            jpeg_image->quality=minimum--;
            status=WriteJPEGImage(extent_info,jpeg_image,exception);
            (void) RelinquishUniqueFileResource(jpeg_image->filename);
            if (status == MagickFalse)
              continue;
            if (GetBlobSize(jpeg_image) <= extent)
              break;
          }
          quality=(int) minimum;
          jpeg_image=DestroyImage(jpeg_image);
        }
      extent_info=DestroyImageInfo(extent_info);
    }
  jpeg_set_quality(jpeg_info,quality,TRUE);
  if ((dct_method == (const char *) NULL) && (quality <= 90))
    jpeg_info->dct_method=JDCT_IFAST;
#if (JPEG_LIB_VERSION >= 70)
  option=GetImageOption(image_info,"quality");
  if (option != (const char *) NULL)
    {
      GeometryInfo
        geometry_info;

      int
        flags;

      /*
        Set quality scaling for luminance and chrominance separately.
      */
      flags=ParseGeometry(option,&geometry_info);
      if (((flags & RhoValue) != 0) && ((flags & SigmaValue) != 0))
        {
          jpeg_info->q_scale_factor[0]=jpeg_quality_scaling((int)
            (geometry_info.rho+0.5));
          jpeg_info->q_scale_factor[1]=jpeg_quality_scaling((int)
            (geometry_info.sigma+0.5));
          jpeg_default_qtables(jpeg_info,TRUE);
        }
    }
#endif
  colorspace=(int) jpeg_info->in_color_space;
  value=GetImageOption(image_info,"jpeg:colorspace");
  if (value == (char *) NULL)
    value=GetImageProperty(image,"jpeg:colorspace",exception);
  if (value != (char *) NULL)
    colorspace=StringToInteger(value);
  sampling_factor=(const char *) NULL;
  if ((J_COLOR_SPACE) colorspace == jpeg_info->in_color_space)
    {
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
    }
  value=GetImageOption(image_info,"jpeg:sampling-factor");
  if (image_info->sampling_factor != (char *) NULL)
    sampling_factor=image_info->sampling_factor;
  if (sampling_factor == (const char *) NULL)
    {
      if (quality >= 90)
        for (i=0; i < MAX_COMPONENTS; i++)
        {
          jpeg_info->comp_info[i].h_samp_factor=1;
          jpeg_info->comp_info[i].v_samp_factor=1;
        }
    }
  else
    {
      char
        **factors;

      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;