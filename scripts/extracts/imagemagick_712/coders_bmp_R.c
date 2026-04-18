// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 18/24



  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(status);
  if (((image->columns << 3) != (size_t) ((int) (image->columns << 3))) ||
      ((image->rows << 3) != (size_t) ((int) (image->rows << 3))))
    ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");
  type=4;
  if (LocaleCompare(image_info->magick,"BMP2") == 0)
    type=2;
  else
    if (LocaleCompare(image_info->magick,"BMP3") == 0)
      type=3;
  option=GetImageOption(image_info,"bmp:format");
  if (option != (char *) NULL)
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "  Format=%s",option);
      if (LocaleCompare(option,"bmp2") == 0)
        type=2;
      if (LocaleCompare(option,"bmp3") == 0)
        type=3;
      if (LocaleCompare(option,"bmp4") == 0)
        type=4;
    }
  scene=0;
  number_scenes=GetImageListLength(image);
  do
  {
    /*
      Initialize BMP raster file header.
    */
    if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
      (void) TransformImageColorspace(image,sRGBColorspace,exception);
    (void) memset(&bmp_info,0,sizeof(bmp_info));
    bmp_info.file_size=14+12;
    if (type > 2)
      bmp_info.file_size+=28;
    bmp_info.offset_bits=bmp_info.file_size;
    bmp_info.compression=BI_RGB;
    bmp_info.red_mask=0x00ff0000U;
    bmp_info.green_mask=0x0000ff00U;
    bmp_info.blue_mask=0x000000ffU;
    bmp_info.alpha_mask=0xff000000U;
    bmp_subtype=UndefinedSubtype;
    if ((image->storage_class == PseudoClass) && (image->colors > 256))
      (void) SetImageStorageClass(image,DirectClass,exception);
    if (image->storage_class != DirectClass)
      {
        /*
          Colormapped BMP raster.
        */
        bmp_info.bits_per_pixel=8;
        if (image->colors <= 2)
          bmp_info.bits_per_pixel=1;
        else
          if (image->colors <= 16)
            bmp_info.bits_per_pixel=4;
          else
            if (image->colors <= 256)
              bmp_info.bits_per_pixel=8;
        if (image_info->compression == RLECompression)
          bmp_info.bits_per_pixel=8;
        bmp_info.number_colors=1U << bmp_info.bits_per_pixel;
        if (image->alpha_trait != UndefinedPixelTrait)
          (void) SetImageStorageClass(image,DirectClass,exception);
        else
          if ((size_t) bmp_info.number_colors < image->colors)
            (void) SetImageStorageClass(image,DirectClass,exception);
          else
            {
              bmp_info.file_size+=3*(1UL << bmp_info.bits_per_pixel);
              bmp_info.offset_bits+=3*(1UL << bmp_info.bits_per_pixel);
              if (type > 2)
                {
                  bmp_info.file_size+=(1UL << bmp_info.bits_per_pixel);
                  bmp_info.offset_bits+=(1UL << bmp_info.bits_per_pixel);
                }
            }
      }
    if (image->storage_class == DirectClass)
      {
        /*
          Full color BMP raster.
        */
        bmp_info.number_colors=0;
        option=GetImageOption(image_info,"bmp:subtype");
        if (option != (const char *) NULL)
        {
          if (image->alpha_trait != UndefinedPixelTrait)
            {
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
        if (bmp_subtyp