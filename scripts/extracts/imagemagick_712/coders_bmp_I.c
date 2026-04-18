// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 9/24

.compression == BI_BITFIELDS) || 
       (bmp_info.compression == BI_ALPHABITFIELDS))) ? BlendPixelTrait :
       UndefinedPixelTrait;
    if (bmp_info.bits_per_pixel < 16)
      {
        size_t
          one;

        image->storage_class=PseudoClass;
        image->colors=bmp_info.number_colors;
        one=1;
        if (image->colors == 0)
          image->colors=one << bmp_info.bits_per_pixel;
      }
    image->resolution.x=(double) bmp_info.x_pixels/100.0;
    image->resolution.y=(double) bmp_info.y_pixels/100.0;
    image->units=PixelsPerCentimeterResolution;
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    status=SetImageExtent(image,image->columns,image->rows,exception);
    if (status == MagickFalse)
      return(DestroyImageList(image));
    if (image->storage_class == PseudoClass)
      {
        unsigned char
          *bmp_colormap;

        size_t
          packet_size;