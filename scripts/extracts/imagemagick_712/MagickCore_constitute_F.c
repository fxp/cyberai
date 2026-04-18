// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/constitute.c
// Segment 6/15



  orientation=(const char *) NULL;
  if (constitute_info->sync_from_exif != MagickFalse)
    {
      orientation=GetImageProperty(image,"exif:Orientation",exception);
      if (orientation != (const char *) NULL)
        {
          image->orientation=(OrientationType) StringToLong(orientation);
          (void) DeleteImageProperty(image,"exif:Orientation");
        }
    }
  if ((orientation == (const char *) NULL) &&
      (constitute_info->sync_from_tiff != MagickFalse))
    {
      orientation=GetImageProperty(image,"tiff:Orientation",exception);
      if (orientation != (const char *) NULL)
        {
          image->orientation=(OrientationType) StringToLong(orientation);
          (void) DeleteImageProperty(image,"tiff:Orientation");
        }
    }
}

static void SyncResolutionFromProperties(Image *image,
  ConstituteInfo *constitute_info, ExceptionInfo *exception)
{
  const char
    *resolution_x,
    *resolution_y,
    *resolution_units;

  MagickBooleanType
    used_tiff;

  resolution_x=(const char *) NULL;
  resolution_y=(const char *) NULL;
  resolution_units=(const char *) NULL;
  used_tiff=MagickFalse;
  if (constitute_info->sync_from_exif != MagickFalse)
    {
      resolution_x=GetImageProperty(image,"exif:XResolution",exception);
      resolution_y=GetImageProperty(image,"exif:YResolution",exception);
      if ((resolution_x != (const char *) NULL) &&
          (resolution_y != (const char *) NULL))
        resolution_units=GetImageProperty(image,"exif:ResolutionUnit",
          exception);
    }
  if ((resolution_x == (const char *) NULL) &&
      (resolution_y == (const char *) NULL) &&
      (constitute_info->sync_from_tiff != MagickFalse))
    {
      resolution_x=GetImageProperty(image,"tiff:XResolution",exception);
      resolution_y=GetImageProperty(image,"tiff:YResolution",exception);
      if ((resolution_x != (const char *) NULL) &&
          (resolution_y != (const char *) NULL))
        {
          used_tiff=MagickTrue;
          resolution_units=GetImageProperty(image,"tiff:ResolutionUnit",
            exception);
        }
    }
  if ((resolution_x != (const char *) NULL) &&
      (resolution_y != (const char *) NULL))
    {
      GeometryInfo
        geometry_info;

      ssize_t
        option_type;

      geometry_info.rho=image->resolution.x;
      geometry_info.sigma=1.0;
      (void) ParseGeometry(resolution_x,&geometry_info);
      if (geometry_info.sigma != 0)
        image->resolution.x=geometry_info.rho/geometry_info.sigma;
      if (strchr(resolution_x,',') != (char *) NULL)
        image->resolution.x=geometry_info.rho+geometry_info.sigma/1000.0;
      geometry_info.rho=image->resolution.y;
      geometry_info.sigma=1.0;
      (void) ParseGeometry(resolution_y,&geometry_info);
      if (geometry_info.sigma != 0)
        image->resolution.y=geometry_info.rho/geometry_info.sigma;
      if (strchr(resolution_y,',') != (char *) NULL)
        image->resolution.y=geometry_info.rho+geometry_info.sigma/1000.0;
      if (resolution_units != (char *) NULL)
        {
          option_type=ParseCommandOption(MagickResolutionOptions,MagickFalse,
            resolution_units);
          if (option_type >= 0)
            image->units=(ResolutionType) option_type;
        }
      if (used_tiff == MagickFalse)
        {
          (void) DeleteImageProperty(image,"exif:XResolution");
          (void) DeleteImageProperty(image,"exif:YResolution");
          (void) DeleteImageProperty(image,"exif:ResolutionUnit");
        }
      else
        {
          (void) DeleteImageProperty(image,"tiff:XResolution");
          (void) DeleteImageProperty(image,"tiff:YResolution");
          (void) DeleteImageProperty(image,"tiff:ResolutionUnit");
        }
    }
}

MagickExport Image *ReadImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    magick[MagickPathExtent],
    magick_filename[MagickPathExtent];

  ConstituteInfo
    constitute_info;

  const DelegateInfo
    *delegate_info;

  const MagickInfo
    *magick_info;

  DecodeImageHandler
    *decoder;

  ExceptionInfo
    *sans_exception;

  Image
    *image,
    *next;

  ImageInfo
    *read_info;

  MagickBooleanType
    status;