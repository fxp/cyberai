// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jxl.c
// Segment 5/11



        (void) memset(&color_encoding,0,sizeof(color_encoding));
        JXLSetFormat(image,&pixel_format,exception);
#if JPEGXL_NUMERIC_VERSION >= JPEGXL_COMPUTE_NUMERIC_VERSION(0,9,0)
        jxl_status=JxlDecoderGetColorAsEncodedProfile(jxl_info,
          JXL_COLOR_PROFILE_TARGET_DATA,&color_encoding);
#else
        jxl_status=JxlDecoderGetColorAsEncodedProfile(jxl_info,&pixel_format,
          JXL_COLOR_PROFILE_TARGET_DATA,&color_encoding);
#endif
        if (jxl_status == JXL_DEC_SUCCESS)
          {
            if (color_encoding.transfer_function == JXL_TRANSFER_FUNCTION_LINEAR)
              {
                image->colorspace=RGBColorspace;
                image->gamma=1.0;
              }
            if (color_encoding.color_space == JXL_COLOR_SPACE_GRAY)
              {
                image->colorspace=GRAYColorspace;
                if (color_encoding.transfer_function == JXL_TRANSFER_FUNCTION_LINEAR)
                  {
                    image->colorspace=LinearGRAYColorspace;
                    image->gamma=1.0;
                  }
              }
            if (color_encoding.white_point == JXL_WHITE_POINT_CUSTOM)
              {
                image->chromaticity.white_point.x=
                  color_encoding.white_point_xy[0];
                image->chromaticity.white_point.y=
                  color_encoding.white_point_xy[1];
              }
            if (color_encoding.primaries == JXL_PRIMARIES_CUSTOM)
              {
                image->chromaticity.red_primary.x=
                  color_encoding.primaries_red_xy[0];
                image->chromaticity.red_primary.y=
                  color_encoding.primaries_red_xy[1];
                image->chromaticity.green_primary.x=
                  color_encoding.primaries_green_xy[0];
                image->chromaticity.green_primary.y=
                  color_encoding.primaries_green_xy[1];
                image->chromaticity.blue_primary.x=
                  color_encoding.primaries_blue_xy[0];
                image->chromaticity.blue_primary.y=
                  color_encoding.primaries_blue_xy[1];
              }
            if (color_encoding.transfer_function == JXL_TRANSFER_FUNCTION_GAMMA)
              image->gamma=color_encoding.gamma;
            switch (color_encoding.rendering_intent)
            {
              case JXL_RENDERING_INTENT_PERCEPTUAL:
              {
                image->rendering_intent=PerceptualIntent;
                break;
              }
              case JXL_RENDERING_INTENT_RELATIVE:
              {
                image->rendering_intent=RelativeIntent;
                break;
              }
              case JXL_RENDERING_INTENT_SATURATION: 
              {
                image->rendering_intent=SaturationIntent;
                break;
              }
              case JXL_RENDERING_INTENT_ABSOLUTE: 
              {
                image->rendering_intent=AbsoluteIntent;
                break;
              }
              default:
              {
                image->rendering_intent=UndefinedIntent;
                break;
              }
            }
          }
        else
          if (jxl_status != JXL_DEC_ERROR)
            break;
#if JPEGXL_NUMERIC_VERSION >= JPEGXL_COMPUTE_NUMERIC_VERSION(0,9,0)
        jxl_status=JxlDecoderGetICCProfileSize(jxl_info,
          JXL_COLOR_PROFILE_TARGET_ORIGINAL,&profile_size);
#else
        jxl_status=JxlDecoderGetICCProfileSize(jxl_info,&pixel_format,
          JXL_COLOR_PROFILE_TARGET_ORIGINAL,&profile_size);
#endif
        if (jxl_status != JXL_DEC_SUCCESS)
          break;
        profile=AcquireProfileStringInfo("icc",profile_size,exception);
        if (profile != (StringInfo *) NULL)
          {
  #if JPEGXL_NUMERIC_VERSION >= JPEGXL_COMPUTE_NUMERIC_VERSION(0,9,0)
            jxl_status=JxlDecoderGetColorAsICCProfile(jxl_info,
              JXL_COLOR_PROFILE_TARGET_ORIGINAL,GetStringInfoDatum(profile),
              profile_size);
#else
            jxl_status=JxlDecoderGetColorAsICCProfile(jxl_info,&pixel_format,
              JXL_COLOR_PROFILE_TARGET_ORIGINAL,GetStringInfoDatum(profile),
              profile_size);
#endif
            if (jxl_status == JXL_DEC_SUCCESS)
              (void) SetImageProfilePrivate(image,profile,exception);
            else
              profile=DestroyStringInfo(profile);
          }
        if (jxl_status == JXL_DEC_SUCCESS)
          jxl_status=JXL_DEC_COLOR_ENCODING;
        break;
      }
      case JXL_DEC_FRAME:
      {
        JxlFrameHeader
          frame_header;