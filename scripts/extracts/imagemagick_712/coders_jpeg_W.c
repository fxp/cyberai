// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 23/24



      /*
        Custom quantization tables.
      */
      table=GetQuantizationTable(option,"0",exception);
      if (table != (QuantizationTable *) NULL)
        {
          for (i=0; i < MAX_COMPONENTS; i++)
            jpeg_info->comp_info[i].quant_tbl_no=0;
          jpeg_add_quant_table(jpeg_info,0,table->levels,
            jpeg_quality_scaling(quality),0);
          table=DestroyQuantizationTable(table);
        }
      table=GetQuantizationTable(option,"1",exception);
      if (table != (QuantizationTable *) NULL)
        {
          for (i=1; i < MAX_COMPONENTS; i++)
            jpeg_info->comp_info[i].quant_tbl_no=1;
          jpeg_add_quant_table(jpeg_info,1,table->levels,
            jpeg_quality_scaling(quality),0);
          table=DestroyQuantizationTable(table);
        }
      table=GetQuantizationTable(option,"2",exception);
      if (table != (QuantizationTable *) NULL)
        {
          for (i=2; i < MAX_COMPONENTS; i++)
            jpeg_info->comp_info[i].quant_tbl_no=2;
          jpeg_add_quant_table(jpeg_info,2,table->levels,
            jpeg_quality_scaling(quality),0);
          table=DestroyQuantizationTable(table);
        }
      table=GetQuantizationTable(option,"3",exception);
      if (table != (QuantizationTable *) NULL)
        {
          for (i=3; i < MAX_COMPONENTS; i++)
            jpeg_info->comp_info[i].quant_tbl_no=3;
          jpeg_add_quant_table(jpeg_info,3,table->levels,
            jpeg_quality_scaling(quality),0);
          table=DestroyQuantizationTable(table);
        }
    }
  jpeg_start_compress(jpeg_info,TRUE);
  if (image->debug != MagickFalse)
    {
      if (image->storage_class == PseudoClass)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Storage class: PseudoClass");
      else
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Storage class: DirectClass");
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Depth: %.20g",
        (double) image->depth);
      if (image->colors != 0)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Number of colors: %.20g",(double) image->colors);
      else
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Number of colors: unspecified");
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "JPEG data precision: %d",(int) jpeg_info->data_precision);
      switch (jpeg_info->in_color_space)
      {
        case JCS_CMYK:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: CMYK");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d,%dx%d,%dx%d,%dx%d",
            jpeg_info->comp_info[0].h_samp_factor,
            jpeg_info->comp_info[0].v_samp_factor,
            jpeg_info->comp_info[1].h_samp_factor,
            jpeg_info->comp_info[1].v_samp_factor,
            jpeg_info->comp_info[2].h_samp_factor,
            jpeg_info->comp_info[2].v_samp_factor,
            jpeg_info->comp_info[3].h_samp_factor,
            jpeg_info->comp_info[3].v_samp_factor);
          break;
        }
        case JCS_GRAYSCALE:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: GRAY");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d",jpeg_info->comp_info[0].h_samp_factor,
            jpeg_info->comp_info[0].v_samp_factor);
          break;
        }
        case JCS_UNKNOWN:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: RGB");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d,%dx%d,%dx%d",
            jpeg_info->comp_info[0].h_samp_factor,
            jpeg_info->comp_info[0].v_samp_factor,
            jpeg_info->comp_info[1].h_samp_factor,
            jpeg_info->comp_info[1].v_samp_factor,
            jpeg_info->comp_info[2].h_samp_factor,
            jpeg_info->comp_info[2].v_samp_factor);
          break;
        }
        case JCS_YCbCr:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: YCbCr");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d,%dx%d,%dx%d",
            jpeg_info->comp_info[0].h_samp_factor,
            jpeg_info->comp_info[0].v_samp_factor,
            jpeg_info->comp_info[1].h_samp_factor,
            jpeg_info->comp_info[1].v_samp_factor,
            jpeg_info->comp_info[2].h_samp_factor,
            jpeg_info->comp_info[2].v_samp_factor);
          break;
        }
        default: break;
      }
    }
  /*
    Write JPEG profiles.
  */
  value=GetImageProperty(image,"comment",exception);
  if (value != (char *) NULL)
    {
      size_t
        length;