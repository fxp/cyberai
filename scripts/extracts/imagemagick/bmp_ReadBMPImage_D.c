/* ===== EXTRACT: bmp.c ===== */
/* Function: ReadBMPImage (part D) */
/* Lines: 967–1080 */

    if (bmp_info.width <= 0)
      ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize");
    if (bmp_info.height == 0)
      ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize");
    if (bmp_info.compression == BI_JPEG)
      {
        /*
          Read embedded JPEG image.
        */
        Image *embed_image = ReadEmbedImage(image_info,image,"jpeg",exception);
        (void) CloseBlob(image);
        image=DestroyImageList(image);
        return(embed_image);
      }
    if (bmp_info.compression == BI_PNG)
      {
        /*
          Read embedded PNG image.
        */
        Image *embed_image = ReadEmbedImage(image_info,image,"png",exception);
        (void) CloseBlob(image);
        image=DestroyImageList(image);
        return(embed_image);
      }
    if (bmp_info.planes != 1)
      ThrowReaderException(CorruptImageError,"StaticPlanesValueNotEqualToOne");
    switch (bmp_info.bits_per_pixel)
    {
      case 1: case 4: case 8: case 16: case 24: case 32: case 64:
      {
        break;
      }
      default:
        ThrowReaderException(CorruptImageError,"UnsupportedBitsPerPixel");
    }
    if ((bmp_info.bits_per_pixel < 16) &&
        (bmp_info.number_colors > (1U << bmp_info.bits_per_pixel)))
      ThrowReaderException(CorruptImageError,"UnrecognizedNumberOfColors");
    if ((MagickSizeType) bmp_info.number_colors > blob_size)
      ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
    if ((bmp_info.compression == BI_RLE8) && (bmp_info.bits_per_pixel != 8))
      ThrowReaderException(CorruptImageError,"UnsupportedBitsPerPixel");
    if ((bmp_info.compression == BI_RLE4) && (bmp_info.bits_per_pixel != 4))
      ThrowReaderException(CorruptImageError,"UnsupportedBitsPerPixel");
    if ((bmp_info.compression == BI_BITFIELDS) &&
        (bmp_info.bits_per_pixel < 16))
      ThrowReaderException(CorruptImageError,"UnsupportedBitsPerPixel");
    switch (bmp_info.compression)
    {
      case BI_RGB:
        image->compression=NoCompression;
        break;
      case BI_RLE8:
      case BI_RLE4:
        image->compression=RLECompression;
        break;
      case BI_BITFIELDS:
        break;
      case BI_ALPHABITFIELDS:
        break;
      case BI_JPEG:
        ThrowReaderException(CoderError,"JPEGCompressNotSupported");
      case BI_PNG:
        ThrowReaderException(CoderError,"PNGCompressNotSupported");
      default:
        ThrowReaderException(CorruptImageError,"UnrecognizedImageCompression");
    }
    image->columns=(size_t) MagickAbsoluteValue(bmp_info.width);
    image->rows=(size_t) MagickAbsoluteValue(bmp_info.height);
    image->depth=bmp_info.bits_per_pixel <= 8 ? bmp_info.bits_per_pixel : 8;
    image->alpha_trait=((bmp_info.alpha_mask != 0) &&
      ((bmp_info.compression == BI_BITFIELDS) || 
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

        /*
          Read BMP raster colormap.
        */
        if (image->debug != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "  Reading colormap of %.20g colors",(double) image->colors);
        if (AcquireImageColormap(image,image->colors,exception) == MagickFalse)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        bmp_colormap=(unsigned char *) AcquireQuantumMemory((size_t)
          image->colors,4*sizeof(*bmp_colormap));
        if (bmp_colormap == (unsigned char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
