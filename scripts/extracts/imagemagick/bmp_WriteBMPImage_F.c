/* ===== EXTRACT: bmp.c ===== */
/* Function: WriteBMPImage (part F) */
/* Lines: 2479–2565 */

    else
      {
        /*
          Write 40-byte version 3+ bitmap header.
        */
        (void) WriteBlobLSBLong(image,bmp_info.size);
        (void) WriteBlobLSBSignedLong(image,(signed int) bmp_info.width);
        (void) WriteBlobLSBSignedLong(image,(signed int) bmp_info.height);
        (void) WriteBlobLSBShort(image,bmp_info.planes);
        (void) WriteBlobLSBShort(image,bmp_info.bits_per_pixel);
        (void) WriteBlobLSBLong(image,bmp_info.compression);
        (void) WriteBlobLSBLong(image,bmp_info.image_size);
        (void) WriteBlobLSBLong(image,bmp_info.x_pixels);
        (void) WriteBlobLSBLong(image,bmp_info.y_pixels);
        (void) WriteBlobLSBLong(image,bmp_info.number_colors);
        (void) WriteBlobLSBLong(image,bmp_info.colors_important);
      }
    if ((type > 3) && ((image->alpha_trait != UndefinedPixelTrait) ||
        (have_color_info != MagickFalse)))
      {
        /*
          Write the rest of the 108-byte BMP Version 4 header.
        */
        (void) WriteBlobLSBLong(image,bmp_info.red_mask);
        (void) WriteBlobLSBLong(image,bmp_info.green_mask);
        (void) WriteBlobLSBLong(image,bmp_info.blue_mask);
        (void) WriteBlobLSBLong(image,bmp_info.alpha_mask);
        if (profile != (StringInfo *) NULL)
          (void) WriteBlobLSBLong(image,0x4D424544U);  /* PROFILE_EMBEDDED */
        else
          (void) WriteBlobLSBLong(image,0x73524742U);  /* sRGB */

        /* bounds check, assign .0 if invalid value */
        if (isgreater(image->chromaticity.red_primary.x, 1.0) ||
            !isgreater(image->chromaticity.red_primary.x, 0.0))
          image->chromaticity.red_primary.x = 0.0;
        if (isgreater(image->chromaticity.red_primary.y, 1.0) ||
            !isgreater(image->chromaticity.red_primary.y, 0.0))
          image->chromaticity.red_primary.y = 0.0;
        if (isgreater(image->chromaticity.green_primary.x, 1.0) ||
            !isgreater(image->chromaticity.green_primary.x, 0.0))
          image->chromaticity.green_primary.x = 0.0;
        if (isgreater(image->chromaticity.green_primary.y, 1.0) ||
            !isgreater(image->chromaticity.green_primary.y, 0.0))
          image->chromaticity.green_primary.y = 0.0;
        if (isgreater(image->chromaticity.blue_primary.x, 1.0) ||
            !isgreater(image->chromaticity.blue_primary.x, 0.0))
          image->chromaticity.blue_primary.x = 0.0;
        if (isgreater(image->chromaticity.blue_primary.y, 1.0) ||
            !isgreater(image->chromaticity.blue_primary.y, 0.0))
          image->chromaticity.blue_primary.y = 0.0;
        if (isgreater(bmp_info.gamma_scale.x, 1.0) ||
            !isgreater(bmp_info.gamma_scale.x, 0.0))
          bmp_info.gamma_scale.x = 0.0;
        if (isgreater(bmp_info.gamma_scale.y, 1.0) ||
            !isgreater(bmp_info.gamma_scale.y, 0.0))
          bmp_info.gamma_scale.y = 0.0;
        if (isgreater(bmp_info.gamma_scale.z, 1.0) ||
            !isgreater(bmp_info.gamma_scale.z, 0.0))
          bmp_info.gamma_scale.z = 0.0;

        (void) WriteBlobLSBLong(image,(unsigned int)
          (image->chromaticity.red_primary.x*0x40000000));
        (void) WriteBlobLSBLong(image,(unsigned int)
          (image->chromaticity.red_primary.y*0x40000000));
        (void) WriteBlobLSBLong(image,(unsigned int)
          ((1.000-(image->chromaticity.red_primary.x+
          image->chromaticity.red_primary.y))*0x40000000));
        (void) WriteBlobLSBLong(image,(unsigned int)
          (image->chromaticity.green_primary.x*0x40000000));
        (void) WriteBlobLSBLong(image,(unsigned int)
          (image->chromaticity.green_primary.y*0x40000000));
        (void) WriteBlobLSBLong(image,(unsigned int)
          ((1.000-(image->chromaticity.green_primary.x+
          image->chromaticity.green_primary.y))*0x40000000));
        (void) WriteBlobLSBLong(image,(unsigned int)
          (image->chromaticity.blue_primary.x*0x40000000));
        (void) WriteBlobLSBLong(image,(unsigned int)
          (image->chromaticity.blue_primary.y*0x40000000));
        (void) WriteBlobLSBLong(image,(unsigned int)
          ((1.000-(image->chromaticity.blue_primary.x+
          image->chromaticity.blue_primary.y))*0x40000000));
        (void) WriteBlobLSBLong(image,(unsigned int)
          (bmp_info.gamma_scale.x*0x10000));
        (void) WriteBlobLSBLong(image,(unsigned int)
          (bmp_info.gamma_scale.y*0x10000));
        (void) WriteBlobLSBLong(image,(unsigned int)
