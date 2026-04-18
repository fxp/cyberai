// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 24/34



      a=QuantumScale*(double) GetPixela(image,q)-0.5;
      if (a < 0.0)
        a+=1.0;
      b=QuantumScale*(double) GetPixelb(image,q)-0.5;
      if (b < 0.0)
        b+=1.0;
      SetPixela(image,(Quantum) (QuantumRange*a),q);
      SetPixelb(image,(Quantum) (QuantumRange*b),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      {
        status=MagickFalse;
        break;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

static MagickBooleanType GetTIFFInfo(const ImageInfo *image_info,
  TIFF *tiff,TIFFInfo *tiff_info)
{
#define TIFFStripSizeDefault  1048576

  const char
    *option;

  MagickStatusType
    flags;

  uint32
    tile_columns,
    tile_rows;

  assert(tiff_info != (TIFFInfo *) NULL);
  (void) memset(tiff_info,0,sizeof(*tiff_info));
  option=GetImageOption(image_info,"tiff:tile-geometry");
  if (option == (const char *) NULL)
    {
      size_t
        extent;

      uint32
        rows,
        rows_per_strip;

      extent=(size_t) TIFFScanlineSize(tiff);
      rows_per_strip=TIFFStripSizeDefault/(extent == 0 ? 1 : (uint32) extent);
      rows_per_strip=16*(((rows_per_strip < 16 ? 16 : rows_per_strip)+1)/16);
      if ((TIFFGetField(tiff,TIFFTAG_IMAGELENGTH,&rows) == 1) &&
          (rows_per_strip > rows))
        rows_per_strip=rows;
      option=GetImageOption(image_info,"tiff:rows-per-strip");
      if (option != (const char *) NULL)
        rows_per_strip=(uint32) strtoul(option,(char **) NULL,10);
      rows_per_strip=TIFFDefaultStripSize(tiff,rows_per_strip);
      (void) TIFFSetField(tiff,TIFFTAG_ROWSPERSTRIP,rows_per_strip);
      return(MagickTrue);
    }
  /*
    Create tiled TIFF, ignore "tiff:rows-per-strip".
  */
  flags=ParseAbsoluteGeometry(option,&tiff_info->tile_geometry);
  if ((flags & HeightValue) == 0)
    tiff_info->tile_geometry.height=tiff_info->tile_geometry.width;
  tile_columns=(uint32) tiff_info->tile_geometry.width;
  tile_rows=(uint32) tiff_info->tile_geometry.height;
  TIFFDefaultTileSize(tiff,&tile_columns,&tile_rows);
  (void) TIFFSetField(tiff,TIFFTAG_TILEWIDTH,tile_columns);
  (void) TIFFSetField(tiff,TIFFTAG_TILELENGTH,tile_rows);
  tiff_info->tile_geometry.width=tile_columns;
  tiff_info->tile_geometry.height=tile_rows;
  if ((TIFFScanlineSize(tiff) <= 0) || (TIFFTileSize(tiff) <= 0))
    {
      DestroyTIFFInfo(tiff_info);
      return(MagickFalse);
    }
  tiff_info->scanlines=(unsigned char *) AcquireQuantumMemory((size_t)
    tile_rows*(size_t) TIFFScanlineSize(tiff),sizeof(*tiff_info->scanlines));
  tiff_info->pixels=(unsigned char *) AcquireQuantumMemory((size_t)
    tile_rows*(size_t) TIFFTileSize(tiff),sizeof(*tiff_info->scanlines));
  if ((tiff_info->scanlines == (unsigned char *) NULL) ||
      (tiff_info->pixels == (unsigned char *) NULL))
    {
      DestroyTIFFInfo(tiff_info);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static int TIFFWritePixels(TIFF *tiff,TIFFInfo *tiff_info,ssize_t row,
  tsample_t sample,Image *image)
{
  tmsize_t
    status;

  size_t
    number_tiles,
    tile_width;

  ssize_t
    bytes_per_pixel,
    i = 0,
    j,
    k,
    l;

  unsigned char
    *p,
    *q;