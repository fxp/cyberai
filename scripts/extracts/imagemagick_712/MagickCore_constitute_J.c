// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/constitute.c
// Segment 10/15



        SetGeometry(next,&geometry);
        flags=ParseAbsoluteGeometry(read_info->extract,&geometry);
        if ((next->columns != geometry.width) ||
            (next->rows != geometry.height))
          {
            if (((flags & XValue) != 0) || ((flags & YValue) != 0))
              {
                Image *crop_image = CropImage(next,&geometry,exception);
                if (crop_image != (Image *) NULL)
                  ReplaceImageInList(&next,crop_image);
              }
            else
              if (((flags & WidthValue) != 0) || ((flags & HeightValue) != 0))
                {
                  flags=ParseRegionGeometry(next,read_info->extract,&geometry,
                    exception);
                  if ((geometry.width != 0) && (geometry.height != 0))
                    {
                      Image *resize_image = ResizeImage(next,geometry.width,
                        geometry.height,next->filter,exception);
                      if (resize_image != (Image *) NULL)
                        ReplaceImageInList(&next,resize_image);
                    }
                }
          }
      }
    profile=GetImageProfile(next,"icc");
    if (profile == (const StringInfo *) NULL)
      profile=GetImageProfile(next,"icm");
    profile=GetImageProfile(next,"iptc");
    if (profile == (const StringInfo *) NULL)
      profile=GetImageProfile(next,"8bim");
    if (IsSourceDataEpochSet() == MagickFalse)
      {
        char
          timestamp[MagickTimeExtent];

        (void) FormatMagickTime(next->timestamp,sizeof(timestamp),timestamp);
        (void) SetImageProperty(next,"date:timestamp",timestamp,exception);
        (void) FormatMagickTime((time_t) GetBlobProperties(next)->st_mtime,
          sizeof(timestamp),timestamp);
        (void) SetImageProperty(next,"date:modify",timestamp,exception);
        (void) FormatMagickTime((time_t) GetBlobProperties(next)->st_ctime,
          sizeof(timestamp),timestamp);
        (void) SetImageProperty(next,"date:create",timestamp,exception);
      }
    if (constitute_info.delay_flags != NoValue)
      {
        if ((constitute_info.delay_flags & GreaterValue) != 0)
          {
            if (next->delay > constitute_info.delay)
              next->delay=constitute_info.delay;
          }
        else
          if ((constitute_info.delay_flags & LessValue) != 0)
            {
              if (next->delay < constitute_info.delay)
                next->delay=constitute_info.delay;
            }
          else
            next->delay=constitute_info.delay;
        if ((constitute_info.delay_flags & SigmaValue) != 0)
          next->ticks_per_second=constitute_info.ticks_per_second;
      }
    if (constitute_info.dispose != (const char *) NULL)
      {
        ssize_t
          option_type;

        option_type=ParseCommandOption(MagickDisposeOptions,MagickFalse,
          constitute_info.dispose);
        if (option_type >= 0)
          next->dispose=(DisposeType) option_type;
      }
    if (read_info->verbose != MagickFalse)
      (void) IdentifyImage(next,stderr,MagickFalse,exception);
    image=next;
  }
  read_info=DestroyImageInfo(read_info);
  if (GetBlobError(image) != MagickFalse)
    ThrowReaderException(CorruptImageError,"UnableToReadImageData");
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d I m a g e s                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadImages() reads one or more images and returns them as an image list.
%
%  The format of the ReadImage method is:
%
%      Image *ReadImages(ImageInfo *image_info,const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o filename: the image filename.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *ReadImages(ImageInfo *image_info,const char *filename,
  ExceptionInfo *exception)
{
  char
    read_filename[MagickPathExtent];

  Image
    *image,
    *images;

  ImageInfo
    *read_info;