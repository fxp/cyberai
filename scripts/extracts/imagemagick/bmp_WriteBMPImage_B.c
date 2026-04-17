/* ===== EXTRACT: bmp.c ===== */
/* Function: WriteBMPImage (part B) */
/* Lines: 1983–2102 */

              if (LocaleNCompare(option,"ARGB4444",8) == 0)
                {
                  bmp_subtype=ARGB4444;
                  bmp_info.red_mask=0x00000f00U;
                  bmp_info.green_mask=0x000000f0U;
                  bmp_info.blue_mask=0x0000000fU;
                  bmp_info.alpha_mask=0x0000f000U;
                }
              else if (LocaleNCompare(option,"ARGB1555",8) == 0)
                {
                  bmp_subtype=ARGB1555;
                  bmp_info.red_mask=0x00007c00U;
                  bmp_info.green_mask=0x000003e0U;
                  bmp_info.blue_mask=0x0000001fU;
                  bmp_info.alpha_mask=0x00008000U;
                }
            }
          else
          {
            if (LocaleNCompare(option,"RGB555",6) == 0)
              {
                bmp_subtype=RGB555;
                bmp_info.red_mask=0x00007c00U;
                bmp_info.green_mask=0x000003e0U;
                bmp_info.blue_mask=0x0000001fU;
                bmp_info.alpha_mask=0U;
              }
            else if (LocaleNCompare(option,"RGB565",6) == 0)
              {
                bmp_subtype=RGB565;
                bmp_info.red_mask=0x0000f800U;
                bmp_info.green_mask=0x000007e0U;
                bmp_info.blue_mask=0x0000001fU;
                bmp_info.alpha_mask=0U;
              }
          }
        }
        if (bmp_subtype != UndefinedSubtype)
          {
            bmp_info.bits_per_pixel=16;
            bmp_info.compression=BI_BITFIELDS;
          }
        else
          {
            bmp_info.bits_per_pixel=(unsigned short) ((type > 3) &&
               (image->alpha_trait != UndefinedPixelTrait) ? 32 : 24);
            bmp_info.compression=(unsigned int) ((type > 3) &&
              (image->alpha_trait != UndefinedPixelTrait) ? BI_BITFIELDS : BI_RGB);
            if ((type == 3) && (image->alpha_trait != UndefinedPixelTrait))
              {
                option=GetImageOption(image_info,"bmp3:alpha");
                if (IsStringTrue(option))
                  bmp_info.bits_per_pixel=32;
              }
          }
      }
    bytes_per_line=4*((image->columns*bmp_info.bits_per_pixel+31)/32);
    bmp_info.ba_offset=0;
    if (type > 3)
      profile=GetImageProfile(image,"icc");
    have_color_info=(image->rendering_intent != UndefinedIntent) ||
      (profile != (StringInfo *) NULL) || (image->gamma != 0.0) ?  MagickTrue :
      MagickFalse;
    if (type == 2)
      bmp_info.size=12;
    else
      if ((type == 3) || (((image->alpha_trait & BlendPixelTrait) == 0) &&
          (have_color_info == MagickFalse)))
        {
          type=3;
          bmp_info.size=40;
        }
      else
        {
          int
            extra_size;

          bmp_info.size=108;
          extra_size=68;
          if ((image->rendering_intent != UndefinedIntent) ||
              (profile != (StringInfo *) NULL))
            {
              bmp_info.size=124;
              extra_size+=16;
            }
          bmp_info.file_size+=(unsigned int) extra_size;
          bmp_info.offset_bits+=(unsigned int) extra_size;
        }
    if (((ssize_t) image->columns != (ssize_t) ((signed int) image->columns)) ||
        ((ssize_t) image->rows != (ssize_t) ((signed int) image->rows)))
      ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");
    bmp_info.width=(ssize_t) image->columns;
    bmp_info.height=(ssize_t) image->rows;
    bmp_info.planes=1;
    bmp_info.image_size=(unsigned int) (bytes_per_line*image->rows);
    bmp_info.file_size+=bmp_info.image_size;
    bmp_info.x_pixels=75*39;
    bmp_info.y_pixels=75*39;
    switch (image->units)
    {
      case UndefinedResolution:
      case PixelsPerInchResolution:
      {
        bmp_info.x_pixels=(unsigned int) (100.0*image->resolution.x/2.54);
        bmp_info.y_pixels=(unsigned int) (100.0*image->resolution.y/2.54);
        break;
      }
      case PixelsPerCentimeterResolution:
      {
        bmp_info.x_pixels=(unsigned int) (100.0*image->resolution.x);
        bmp_info.y_pixels=(unsigned int) (100.0*image->resolution.y);
        break;
      }
    }
    bmp_info.colors_important=bmp_info.number_colors;
    /*
      Convert MIFF to BMP raster pixels.
    */
    pixel_info=AcquireVirtualMemory(image->rows,MagickMax(bytes_per_line,
      image->columns+256UL)*sizeof(*pixels));
