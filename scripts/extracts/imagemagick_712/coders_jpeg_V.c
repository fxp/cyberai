// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 22/24



      /*
        Set sampling factor.
      */
      i=0;
      factors=SamplingFactorToList(sampling_factor);
      if (factors != (char **) NULL)
        {
          for (i=0; i < MAX_COMPONENTS; i++)
          {
            if (factors[i] == (char *) NULL)
              break;
            flags=ParseGeometry(factors[i],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=geometry_info.rho;
            jpeg_info->comp_info[i].h_samp_factor=(int) geometry_info.rho;
            jpeg_info->comp_info[i].v_samp_factor=(int) geometry_info.sigma;
            factors[i]=(char *) RelinquishMagickMemory(factors[i]);
          }
          factors=(char **) RelinquishMagickMemory(factors);
        }
      for ( ; i < MAX_COMPONENTS; i++)
      {
        jpeg_info->comp_info[i].h_samp_factor=1;
        jpeg_info->comp_info[i].v_samp_factor=1;
      }
    }
  option=GetImageOption(image_info,"jpeg:q-table");
  if (option != (const char *) NULL)
    {
      QuantizationTable
        *table;