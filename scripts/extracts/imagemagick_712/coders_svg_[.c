// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 27/39



      /*
        Draw image.
      */
      image=DestroyImage(image);
      image=(Image *) NULL;
      read_info=CloneImageInfo(image_info);
      SetImageInfoBlob(read_info,(void *) NULL,0);
      (void) FormatLocaleString(read_info->filename,MagickPathExtent,"mvg:%s",
        filename);
      image=ReadImage(read_info,exception);
      read_info=DestroyImageInfo(read_info);
      if (image != (Image *) NULL)
        (void) CopyMagickString(image->filename,image_info->filename,
          MagickPathExtent);
    }
  /*
    Relinquish resources.
  */
  if (image != (Image *) NULL)
    {
      if (svg_info->title != (char *) NULL)
        (void) SetImageProperty(image,"svg:title",svg_info->title,exception);
      if (svg_info->comment != (char *) NULL)
        (void) SetImageProperty(image,"svg:comment",svg_info->comment,
          exception);
      for (next=GetFirstImageInList(image); next != (Image *) NULL; )
      {
        (void) CopyMagickString(next->filename,image->filename,
          MagickPathExtent);
        (void) CopyMagickString(next->magick,"SVG",MagickPathExtent);
        next=GetNextImageInList(next);
      }
    }
  svg_info=DestroySVGInfo(svg_info);
  (void) RelinquishUniqueFileResource(filename);
  return(GetFirstImageInList(image));
}
#else
static Image *RenderMSVGImage(const ImageInfo *magick_unused(image_info),
  Image *image,ExceptionInfo *magick_unused(exception))
{
  magick_unreferenced(image_info);
  magick_unreferenced(exception);
  image=DestroyImageList(image);
  return((Image *) NULL);
}
#endif

static Image *ReadSVGImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image
    *image;

  MagickBooleanType
    status;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  if ((fabs(image->resolution.x) < MagickEpsilon) ||
      (fabs(image->resolution.y) < MagickEpsilon))
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      flags=ParseGeometry(SVGDensityGeometry,&geometry_info);
      if ((flags & RhoValue) != 0)
        image->resolution.x=geometry_info.rho;
      image->resolution.y=image->resolution.x;
      if ((flags & SigmaValue) != 0)
        image->resolution.y=geometry_info.sigma;
    }
  if (LocaleCompare(image_info->magick,"MSVG") != 0)
    {
      if (LocaleCompare(image_info->magick,"RSVG") != 0)
        {
          Image
            *svg_image;

          svg_image=RenderSVGImage(image_info,image,exception);
          if (svg_image != (Image *) NULL)
            {
              image=DestroyImageList(image);
              return(svg_image);
            }
        }
#if defined(MAGICKCORE_RSVG_DELEGATE)
      if (rsvg_semaphore == (SemaphoreInfo *) NULL)
        ActivateSemaphoreInfo(&rsvg_semaphore);
      LockSemaphoreInfo(rsvg_semaphore);
      image=RenderRSVGImage(image_info,image,exception);
      UnlockSemaphoreInfo(rsvg_semaphore);
      return(image);
#endif
    }
  status=IsRightsAuthorized(CoderPolicyDomain,ReadPolicyRights,"MSVG");
  if (status == MagickFalse)
    image=DestroyImageList(image);
  else
    image=RenderMSVGImage(image_info,image,exception);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r S V G I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterSVGImage() adds attributes for the SVG image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterSVGImage method is:
%
%      size_t RegisterSVGImage(void)
%
*/
ModuleExport size_t RegisterSVGImage(void)
{
  char
    version[MagickPathExtent];

  MagickInfo
    *entry;