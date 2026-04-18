// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/constitute.c
// Segment 5/15



      /*
        Images of the form image-%d.png[1-5].
      */
      read_info=CloneImageInfo(image_info);
      sans=AcquireExceptionInfo();
      (void) SetImageInfo(read_info,0,sans);
      sans=DestroyExceptionInfo(sans);
      if (read_info->number_scenes == 0)
        {
          read_info=DestroyImageInfo(read_info);
          return(PingImage(image_info,exception));
        }
      (void) CopyMagickString(ping_filename,read_info->filename,
        MagickPathExtent);
      images=NewImageList();
      extent=(ssize_t) (read_info->scene+read_info->number_scenes);
      for (scene=(ssize_t) read_info->scene; scene < (ssize_t) extent; scene++)
      {
        (void) InterpretImageFilename(image_info,(Image *) NULL,ping_filename,
          (int) scene,read_info->filename,exception);
        image=PingImage(read_info,exception);
        if (image == (Image *) NULL)
          continue;
        AppendImageToList(&images,image);
      }
      read_info=DestroyImageInfo(read_info);
      return(images);
    }
  return(PingImage(image_info,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadImage() reads an image or image sequence from a file or file handle.
%  The method returns a NULL if there is a memory shortage or if the image
%  cannot be read.  On failure, a NULL image is returned and exception
%  describes the reason for the failure.
%
%  The format of the ReadImage method is:
%
%      Image *ReadImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: Read the image defined by the file or filename members of
%      this structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickBooleanType IsCoderAuthorized(const char *module,
  const char *coder,const PolicyRights rights,ExceptionInfo *exception)
{
  if (IsRightsAuthorized(CoderPolicyDomain,rights,coder) == MagickFalse)
    {
      errno=EPERM;
      (void) ThrowMagickException(exception,GetMagickModule(),PolicyError,
        "NotAuthorized","`%s'",coder);
      return(MagickFalse);
    }
  if (IsRightsAuthorized(ModulePolicyDomain,rights,module) == MagickFalse)
    {
      errno=EPERM;
      (void) ThrowMagickException(exception,GetMagickModule(),PolicyError,
        "NotAuthorized","`%s'",module);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void InitializeConstituteInfo(const ImageInfo *image_info,
  ConstituteInfo *constitute_info)
{
  const char
    *option;

  memset(constitute_info,0,sizeof(*constitute_info));
  constitute_info->sync_from_exif=MagickTrue;
  constitute_info->sync_from_tiff=MagickTrue;
  option=GetImageOption(image_info,"exif:sync-image");
  if (IsStringFalse(option) != MagickFalse)
    constitute_info->sync_from_exif=MagickFalse;
  option=GetImageOption(image_info,"tiff:sync-image");
  if (IsStringFalse(option) != MagickFalse)
    constitute_info->sync_from_tiff=MagickFalse;
  constitute_info->caption=GetImageOption(image_info,"caption");
  constitute_info->comment=GetImageOption(image_info,"comment");
  constitute_info->label=GetImageOption(image_info,"label");
  option=GetImageOption(image_info,"delay");
  if (option != (const char *) NULL)
    {
      GeometryInfo
        geometry_info;

      constitute_info->delay_flags=ParseGeometry(option,&geometry_info);
      if (constitute_info->delay_flags != NoValue)
        {
          constitute_info->delay=(size_t) floor(geometry_info.rho+0.5);
          if ((constitute_info->delay_flags & SigmaValue) != 0)
            constitute_info->ticks_per_second=CastDoubleToSsizeT(floor(
              geometry_info.sigma+0.5));
        }
    }
}

static void SyncOrientationFromProperties(Image *image,
  ConstituteInfo *constitute_info,ExceptionInfo *exception)
{
  const char
    *orientation;