// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 19/24

e != UndefinedSubtype)
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
    if (HeapOverflowSanityCheckGetSize(image->columns,bmp_info.bits_per_pixel,&extent) != MagickFalse)
      ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");
    if (HeapOverflowSanityCheckGetSize(4,((extent+31)/32),&bytes_per_line) != MagickFalse)
      ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");
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
        bmp_info.x_pixels=CastDoubleToUInt(100.0*image->resolution.x/2.54);
        bmp_info.y_pixels=CastDoubleToUInt(100.0*image->resolution.y/2.54);
        break;
      }
      case PixelsPerCentimeterResolution:
      {
        bmp_info.x_pixels=CastDoubleToUInt(100.0*image->resolution.x);
        bmp_info.y_pixels=CastDoubleToUInt(100.0*image->resolution.y);
        break;
      }
    }
    bmp_info.colors_important=bmp_info.number_colors;
    /*
      Convert MIFF to BMP raster pixels.
    */
    extent=MagickMax(bytes_per_line,image->columns+1UL)*
      ((bmp_info.bits_per_pixel+7)/8);
    if (HeapOverflowSanityCheck(extent,sizeof(*pixels)) != MagickFalse)
      ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");
    pixel_info=AcquireVirtualMemory(image->rows,extent*sizeof(*pixels));
    if (pixel_info == (MemoryInfo *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
    (void) memset(pixels,0,(size_t) bmp_info.image_size);
    switch (bmp_info.bits_per_pixel)
    {
      case 1:
      {
        size_t
          bit,
          byte;

        /*
          Convert PseudoClass image to a BMP monochrome image.
        */
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          ssize_t
            offset;