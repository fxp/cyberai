// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 5/30



  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  /*
    Open image file.
  */
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  status=AcquireUniqueSymbolicLink(image_info->filename,input_filename);
  if (status == MagickFalse)
    {
      ThrowFileException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        image_info->filename);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Set the page density.
  */
  delta.x=DefaultResolution;
  delta.y=DefaultResolution;
  (void) memset(&page,0,sizeof(page));
  if ((image->resolution.x == 0.0) || (image->resolution.y == 0.0))
    {
      flags=ParseGeometry(PSDensityGeometry,&geometry_info);
      if ((flags & RhoValue) != 0)
        image->resolution.x=geometry_info.rho;
      image->resolution.y=image->resolution.x;
      if ((flags & SigmaValue) != 0)
        image->resolution.y=geometry_info.sigma;
    }
  if (image_info->density != (char *) NULL)
    {
      flags=ParseGeometry(image_info->density,&geometry_info);
      if ((flags & RhoValue) != 0)
        image->resolution.x=geometry_info.rho;
      image->resolution.y=image->resolution.x;
      if ((flags & SigmaValue) != 0)
        image->resolution.y=geometry_info.sigma;
    }
  (void) ParseAbsoluteGeometry(PSPageGeometry,&page);
  if (image_info->page != (char *) NULL)
    (void) ParseAbsoluteGeometry(image_info->page,&page);
  page.width=(size_t) ((ssize_t) ceil((double) (page.width*
    image->resolution.x/delta.x)-0.5));
  page.height=(size_t) ((ssize_t) ceil((double) (page.height*
    image->resolution.y/delta.y)-0.5));
  /*
    Determine page geometry from the PDF media box.
  */
  ReadPDFInfo(image_info,image,&pdf_info,exception);
  (void) CloseBlob(image);
  /*
    Set PDF render geometry.
  */
  if ((fabs(pdf_info.bounds.x2-pdf_info.bounds.x1) >= MagickEpsilon) &&
      (fabs(pdf_info.bounds.y2-pdf_info.bounds.y1) >= MagickEpsilon))
    {
      (void) FormatImageProperty(image,"pdf:HiResBoundingBox",
        "%gx%g%+.15g%+.15g",pdf_info.bounds.x2-pdf_info.bounds.x1,
        pdf_info.bounds.y2-pdf_info.bounds.y1,pdf_info.bounds.x1,
        pdf_info.bounds.y1);
      page.width=CastDoubleToSizeT(ceil((double) ((pdf_info.bounds.x2-
        pdf_info.bounds.x1)*image->resolution.x/delta.x)-0.5));
      page.height=CastDoubleToSizeT(ceil((double) ((pdf_info.bounds.y2-
        pdf_info.bounds.y1)*image->resolution.y/delta.y)-0.5));
    }
  fitPage=MagickFalse;
  option=GetImageOption(image_info,"pdf:fit-page");
  if (option != (char *) NULL)
    {
      char
        *page_geometry;

      page_geometry=GetPageGeometry(option);
      flags=ParseMetaGeometry(page_geometry,&page.x,&page.y,&page.width,
        &page.height);
      page_geometry=DestroyString(page_geometry);
      if (flags == NoValue)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
            "InvalidGeometry","`%s'",option);
          CleanupPDFInfo(&pdf_info);
          image=DestroyImage(image);
          return((Image *) NULL);
        }
      page.width=(size_t) ((ssize_t) ceil((double) (page.width*
        image->resolution.x/delta.x)-0.5));
      page.height=(size_t) ((ssize_t) ceil((double) (page.height*
        image->resolution.y/delta.y)-0.5));
      fitPage=MagickTrue;
    }
  if ((fabs(pdf_info.angle) == 90.0) || (fabs(pdf_info.angle) == 270.0))
    {
      size_t
        swap;