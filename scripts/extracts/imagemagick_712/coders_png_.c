// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 64/94



  save_image_depth=image_depth;
  ping_bit_depth=(png_byte) save_image_depth;


#if defined(PNG_pHYs_SUPPORTED)
  if (mng_info->exclude_pHYs == MagickFalse)
  {
  if ((image->resolution.x != 0) && (image->resolution.y != 0) &&
      (mng_info->write_mng == MagickFalse || mng_info->equal_physs == MagickFalse))
    {
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Setting up pHYs chunk");

      if (image->units == PixelsPerInchResolution)
        {
          ping_pHYs_unit_type=PNG_RESOLUTION_METER;
          ping_pHYs_x_resolution=(png_uint_32) CastDoubleToSizeT((100.0*
            image->resolution.x)/2.54);
          ping_pHYs_y_resolution=(png_uint_32) CastDoubleToSizeT((100.0*
            image->resolution.y+0.5)/2.54);
        }

      else if (image->units == PixelsPerCentimeterResolution)
        {
          ping_pHYs_unit_type=PNG_RESOLUTION_METER;
          ping_pHYs_x_resolution=(png_uint_32) CastDoubleToSizeT(100.0*
            image->resolution.x);
          ping_pHYs_y_resolution=(png_uint_32) CastDoubleToSizeT(100.0*
            image->resolution.y);
        }

      else
        {
          ping_pHYs_unit_type=PNG_RESOLUTION_UNKNOWN;
          ping_pHYs_x_resolution=(png_uint_32) CastDoubleToSizeT(
            image->resolution.x);
          ping_pHYs_y_resolution=(png_uint_32) CastDoubleToSizeT(
            image->resolution.y);
        }

      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    Set up PNG pHYs chunk: xres: %.20g, yres: %.20g, units: %d.",
          (double) ping_pHYs_x_resolution,(double) ping_pHYs_y_resolution,
          (int) ping_pHYs_unit_type);
       ping_have_pHYs = MagickTrue;
    }
  }
#endif

  if (mng_info->exclude_bKGD == MagickFalse)
  {
  if ((mng_info->adjoin == MagickFalse || mng_info->equal_backgrounds == MagickFalse))
    {
       unsigned int
         mask;

       mask=0xffff;
       if (ping_bit_depth == 8)
          mask=0x00ff;

       if (ping_bit_depth == 4)
          mask=0x000f;

       if (ping_bit_depth == 2)
          mask=0x0003;

       if (ping_bit_depth == 1)
          mask=0x0001;

       ping_background.red=(png_uint_16)
         (ScaleQuantumToShort((Quantum) image->background_color.red) & mask);

       ping_background.green=(png_uint_16)
         (ScaleQuantumToShort((Quantum) image->background_color.green) & mask);

       ping_background.blue=(png_uint_16)
         (ScaleQuantumToShort((Quantum) image->background_color.blue) & mask);

       ping_background.gray=(png_uint_16) ping_background.green;
    }

  if (logging != MagickFalse)
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    Setting up bKGD chunk (1)");
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "      background_color index is %d",
          (int) ping_background.index);

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    ping_bit_depth=%d",ping_bit_depth);
    }

  ping_have_bKGD = MagickTrue;
  }

  /*
    Select the color type.
  */
  matte=image_matte;
  old_bit_depth=0;

  if ((mng_info->is_palette != MagickFalse) &&
      (mng_info->write_png8 != MagickFalse))
    {
      /* To do: make this a function cause it's used twice, except
         for reducing the sample depth from 8. */

      number_colors=image_colors;

      ping_have_tRNS=MagickFalse;

      /*
        Set image palette.
      */
      ping_color_type=(png_byte) PNG_COLOR_TYPE_PALETTE;

      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "  Setting up PLTE chunk with %d colors (%d)",
            number_colors, image_colors);

      for (i=0; i < (ssize_t) number_colors; i++)
      {
        palette[i].red=ScaleQuantumToChar((Quantum) image->colormap[i].red);
        palette[i].green=ScaleQuantumToChar((Quantum) image->colormap[i].green);
        palette[i].blue=ScaleQuantumToChar((Quantum) image->colormap[i].blue);
        if (logging != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
#if MAGICKCORE_QUANTUM_DEPTH == 8
            "    %3ld (%3d,%3d,%3d)",
#else
            "    %5ld (%5d,%5d,%5d)",
#endif
            (long) i,palette[i].red,palette[i].green,palette[i].blue);

      }

      ping_have_PLTE=MagickTrue;
      image_depth=(size_t) ping_bit_depth;
      ping_num_trans=0;

      if (matte != MagickFalse)
      {
          /*
            Identify which colormap entry is transparent.
          */
          assert(number_colors <= 256);
          assert(image->colormap != NULL);

          for (i=0; i < (ssize_t) number_transparent; i++)
             ping_trans_alpha[i]=0;


          ping_num_trans=(unsigned short) (number_transparent +
             number_semitransparent);

          if (ping_num_trans == 0)
             ping_have_tRNS=MagickFalse;

          else
             ping_have_tRNS=MagickTrue;
      }