// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 25/34



  if ((TIFFIsTiled(tiff) == 0) || (tiff_info->tile_geometry.height == 0))
    return(TIFFWriteScanline(tiff,tiff_info->scanline,(uint32) row,sample));
  if (tiff_info->scanline != NULL)
    {
      /*
        Fill scanlines to tile height.
      */
      i=(ssize_t) (row % (ssize_t) tiff_info->tile_geometry.height)*
        TIFFScanlineSize(tiff);
      (void) memcpy(tiff_info->scanlines+i,(char *) tiff_info->scanline,
        (size_t) TIFFScanlineSize(tiff));
      if ((((size_t) row % tiff_info->tile_geometry.height) != (tiff_info->tile_geometry.height-1)) &&
          (row != (ssize_t) (image->rows-1)))
        return(0);
    }
  /*
    Write tile to TIFF image.
  */
  status=0;
  bytes_per_pixel=TIFFTileSize(tiff)/(ssize_t) (
    tiff_info->tile_geometry.height*tiff_info->tile_geometry.width);
  number_tiles=(image->columns+tiff_info->tile_geometry.width)/
    tiff_info->tile_geometry.width;
  for (i=0; i < (ssize_t) number_tiles; i++)
  {
    tile_width=(size_t) ((i == (ssize_t) (number_tiles-1)) ? (ssize_t)
      image->columns-(i*(ssize_t) tiff_info->tile_geometry.width) :
      (ssize_t) tiff_info->tile_geometry.width);
    for (j=0; j < ((row % (ssize_t) tiff_info->tile_geometry.height)+1); j++)
      for (k=0; k < (ssize_t) tile_width; k++)
      {
        if (bytes_per_pixel == 0)
          {
            p=tiff_info->scanlines+(j*TIFFScanlineSize(tiff)+(i*
              (ssize_t) tiff_info->tile_geometry.width+k)/8);
            q=tiff_info->pixels+(j*TIFFTileRowSize(tiff)+k/8);
            *q++=(*p++);
            continue;
          }
        p=tiff_info->scanlines+(j*TIFFScanlineSize(tiff)+(i*
          (ssize_t) tiff_info->tile_geometry.width+k)*bytes_per_pixel);
        q=tiff_info->pixels+(j*TIFFTileRowSize(tiff)+k*bytes_per_pixel);
        for (l=0; l < bytes_per_pixel; l++)
          *q++=(*p++);
      }
    if (((size_t) i*tiff_info->tile_geometry.width) != image->columns)
      status=TIFFWriteTile(tiff,tiff_info->pixels,(uint32) ((size_t) i*
        tiff_info->tile_geometry.width),(uint32) (((size_t) row/
        tiff_info->tile_geometry.height)*tiff_info->tile_geometry.height),0,
        sample);
    if (status < 0)
      break;
  }
  return((int) status);
}

static ssize_t TIFFWriteCustomStream(unsigned char *data,const size_t count,
  void *user_data)
{
  PhotoshopProfile
    *profile;

  if (count == 0)
    return(0);
  profile=(PhotoshopProfile *) user_data;
  if ((profile->offset+(MagickOffsetType) count) >=
        (MagickOffsetType) profile->extent)
    {
      profile->extent+=count+profile->quantum;
      profile->quantum<<=1;
      SetStringInfoLength(profile->data,profile->extent);
    }
  (void) memcpy(profile->data->datum+profile->offset,data,count);
  profile->offset+=(MagickOffsetType) count;
  return((ssize_t) count);
}

static CustomStreamInfo *TIFFAcquireCustomStreamForWriting(
  PhotoshopProfile *profile,ExceptionInfo *exception)
{
  CustomStreamInfo
    *custom_stream;

  custom_stream=AcquireCustomStreamInfo(exception);
  if (custom_stream == (CustomStreamInfo *) NULL)
    return(custom_stream);
  SetCustomStreamData(custom_stream,(void *) profile);
  SetCustomStreamWriter(custom_stream,TIFFWriteCustomStream);
  SetCustomStreamSeeker(custom_stream,TIFFSeekCustomStream);
  SetCustomStreamTeller(custom_stream,TIFFTellCustomStream);
  return(custom_stream);
}

static MagickBooleanType TIFFWritePhotoshopLayers(Image* image,
  const ImageInfo *image_info,EndianType endian,ExceptionInfo *exception)
{
  BlobInfo
    *blob;

  CustomStreamInfo
    *custom_stream;

  Image
    *base_image,
    *next;

  ImageInfo
    *clone_info;

  MagickBooleanType
    status;

  PhotoshopProfile
    profile;

  PSDInfo
    info;

  StringInfo
    *layers;