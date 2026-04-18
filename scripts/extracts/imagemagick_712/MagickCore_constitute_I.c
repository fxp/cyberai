// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/constitute.c
// Segment 9/15

 (read_info->temporary != MagickFalse)
    {
      (void) RelinquishUniqueFileResource(read_info->filename);
      read_info->temporary=MagickFalse;
      if (image != (Image *) NULL)
        (void) CopyMagickString(image->filename,filename,MagickPathExtent);
    }
  if (image == (Image *) NULL)
    {
      read_info=DestroyImageInfo(read_info);
      return(image);
    }
  if (exception->severity >= ErrorException)
    (void) LogMagickEvent(ExceptionEvent,GetMagickModule(),
      "Coder (%s) generated an image despite an error (%d), "
      "notify the developers",image->magick,exception->severity);
  if (IsBlobTemporary(image) != MagickFalse)
    (void) RelinquishUniqueFileResource(read_info->filename);
  if ((IsSceneGeometry(read_info->scenes,MagickFalse) != MagickFalse) &&
      (GetImageListLength(image) != 1))
    {
      Image
        *clones;

      clones=CloneImages(image,read_info->scenes,exception);
      image=DestroyImageList(image);
      if (clones != (Image *) NULL)
        image=GetFirstImageInList(clones);
      if (image == (Image *) NULL)
        {
          read_info=DestroyImageInfo(read_info);
          return(image);
        }
    }
  InitializeConstituteInfo(read_info,&constitute_info);
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    char
      magick_path[MagickPathExtent],
      *property;

    const StringInfo
      *profile;

    next->taint=MagickFalse;
    GetPathComponent(magick_filename,MagickPath,magick_path);
    if ((*magick_path == '\0') && (*next->magick == '\0'))
      (void) CopyMagickString(next->magick,magick,MagickPathExtent);
    (void) CopyMagickString(next->magick_filename,magick_filename,
      MagickPathExtent);
    if (IsBlobTemporary(image) != MagickFalse)
      (void) CopyMagickString(next->filename,filename,MagickPathExtent);
    if (next->magick_columns == 0)
      next->magick_columns=next->columns;
    if (next->magick_rows == 0)
      next->magick_rows=next->rows;
    (void) GetImageProperty(next,"exif:*",exception);
    (void) GetImageProperty(next,"icc:*",exception);
    (void) GetImageProperty(next,"iptc:*",exception);
    (void) GetImageProperty(next,"xmp:*",exception);
    SyncOrientationFromProperties(next,&constitute_info,exception);
    SyncResolutionFromProperties(next,&constitute_info,exception);
    if (next->page.width == 0)
      next->page.width=next->columns;
    if (next->page.height == 0)
      next->page.height=next->rows;
    if (constitute_info.caption != (const char *) NULL)
      {
        property=InterpretImageProperties(read_info,next,
          constitute_info.caption,exception);
        (void) SetImageProperty(next,"caption",property,exception);
        property=DestroyString(property);
      }
    if (constitute_info.comment != (const char *) NULL)
      {
        property=InterpretImageProperties(read_info,next,
          constitute_info.comment,exception);
        (void) SetImageProperty(next,"comment",property,exception);
        property=DestroyString(property);
      }
    if (constitute_info.label != (const char *) NULL)
      {
        property=InterpretImageProperties(read_info,next,
          constitute_info.label,exception);
        (void) SetImageProperty(next,"label",property,exception);
        property=DestroyString(property);
      }
    if (LocaleCompare(next->magick,"TEXT") == 0)
      (void) ParseAbsoluteGeometry("0x0+0+0",&next->page);
    if ((read_info->extract != (char *) NULL) &&
        (read_info->stream == (StreamHandler) NULL))
      {
        RectangleInfo
          geometry;

        MagickStatusType
          flags;