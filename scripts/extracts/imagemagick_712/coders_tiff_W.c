// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 23/34

o *image_info,
  Image *image,ExceptionInfo *exception)
{
  const char
    *option;

  Image
    *images,
    *next,
    *pyramid_image;

  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  PointInfo
    resolution;

  size_t
    columns,
    min_base = 64,
    max_levels = ~0UL,
    rows;

  /*
    Create pyramid-encoded TIFF image.
  */
  option=GetImageOption(image_info,"ptif:pyramid");
  if (option != (const char *) NULL)
    {
      /*
        Property ptif:min-base[x][max-levels].
      */
      RectangleInfo
        pyramid_geometry = { 0, 0, 0, 0 };

      MagickStatusType flags =
        ParseAbsoluteGeometry(option,&pyramid_geometry);
      if ((flags & WidthValue) != 0)
        min_base=pyramid_geometry.width;
      if ((flags & HeightValue) != 0)
        max_levels=pyramid_geometry.height;
    }
  images=NewImageList();
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    Image
      *clone_image;

    ssize_t
      i;

    clone_image=CloneImage(next,0,0,MagickFalse,exception);
    if (clone_image == (Image *) NULL)
      break;
    clone_image->previous=NewImageList();
    clone_image->next=NewImageList();
    (void) SetImageProperty(clone_image,"tiff:subfiletype","none",exception);
    AppendImageToList(&images,clone_image);
    columns=next->columns;
    rows=next->rows;
    resolution=next->resolution;
    for (i=0; (columns > min_base) && (rows > min_base); i++)
    {
      if (i > (ssize_t) max_levels)
        break;
      columns/=2;
      rows/=2;
      resolution.x/=2;
      resolution.y/=2;
      pyramid_image=ResizeImage(next,columns,rows,image->filter,exception);
      if (pyramid_image == (Image *) NULL)
        break;
      DestroyBlob(pyramid_image);
      pyramid_image->blob=ReferenceBlob(next->blob);
      pyramid_image->resolution=resolution;
      (void) SetImageProperty(pyramid_image,"tiff:subfiletype","REDUCEDIMAGE",
        exception);
      AppendImageToList(&images,pyramid_image);
    }
  }
  status=MagickFalse;
  if (images != (Image *) NULL)
    {
      /*
        Write pyramid-encoded TIFF image.
      */
      images=GetFirstImageInList(images);
      write_info=CloneImageInfo(image_info);
      write_info->adjoin=MagickTrue;
      (void) CopyMagickString(write_info->magick,"TIFF",MagickPathExtent);
      (void) CopyMagickString(images->magick,"TIFF",MagickPathExtent);
      status=WriteTIFFImage(write_info,images,exception);
      images=DestroyImageList(images);
      write_info=DestroyImageInfo(write_info);
    }
  return(status);
}
#endif

#if defined(MAGICKCORE_TIFF_DELEGATE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e T I F F I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteTIFFImage() writes an image in the Tagged image file format.
%
%  The format of the WriteTIFFImage method is:
%
%      MagickBooleanType WriteTIFFImage(const ImageInfo *image_info,
%        Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image:  The image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef struct _TIFFInfo
{
  RectangleInfo
    tile_geometry;

  unsigned char
    *scanline,
    *scanlines,
    *pixels;
} TIFFInfo;

static void DestroyTIFFInfo(TIFFInfo *tiff_info)
{
  assert(tiff_info != (TIFFInfo *) NULL);
  if (tiff_info->scanlines != (unsigned char *) NULL)
    tiff_info->scanlines=(unsigned char *) RelinquishMagickMemory(
      tiff_info->scanlines);
  if (tiff_info->pixels != (unsigned char *) NULL)
    tiff_info->pixels=(unsigned char *) RelinquishMagickMemory(
      tiff_info->pixels);
}

static MagickBooleanType EncodeLabImage(Image *image,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        break;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      double
        a,
        b;