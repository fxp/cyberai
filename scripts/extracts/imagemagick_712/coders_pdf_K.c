// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 11/30



  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  pocket_mod=NewImageList();
  pages=NewImageList();
  i=0;
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    Image
      *page;

    if ((i == 0) || (i == 5) || (i == 6) || (i == 7))
      page=RotateImage(next,180.0,exception);
    else
      page=CloneImage(next,0,0,MagickTrue,exception);
    if (page == (Image *) NULL)
      break;
    (void) SetImageAlphaChannel(page,RemoveAlphaChannel,exception);
    page->scene=(size_t) i++;
    AppendImageToList(&pages,page);
    if ((i == 8) || (GetNextImageInList(next) == (Image *) NULL))
      {
        Image
          *images,
          *page_layout;

        MontageInfo
          *montage_info;

        /*
          Create PocketMod page.
        */
        for (i=(ssize_t) GetImageListLength(pages); i < 8; i++)
        {
          page=CloneImage(pages,0,0,MagickTrue,exception);
          (void) QueryColorCompliance("#FFF",AllCompliance,
            &page->background_color,exception);
          (void) SetImageBackgroundColor(page,exception);
          page->scene=(size_t) i;
          AppendImageToList(&pages,page);
        }
        images=CloneImages(pages,PocketPageOrder,exception);
        pages=DestroyImageList(pages);
        if (images == (Image *) NULL)
          break;
        montage_info=CloneMontageInfo(image_info,(MontageInfo *) NULL);
        (void) CloneString(&montage_info->geometry,"877x1240+0+0");
        (void) CloneString(&montage_info->tile,"4x2");
        (void) QueryColorCompliance("#000",AllCompliance,
          &montage_info->border_color,exception);
        montage_info->border_width=2;
        page_layout=MontageImages(images,montage_info,exception);
        montage_info=DestroyMontageInfo(montage_info);
        images=DestroyImageList(images);
        if (page_layout == (Image *) NULL)
          break;
        AppendImageToList(&pocket_mod,page_layout);
        i=0;
      }
  }
  if (pocket_mod == (Image *) NULL)
    return(MagickFalse);
  status=WritePDFImage(image_info,GetFirstImageInList(pocket_mod),exception);
  pocket_mod=DestroyImageList(pocket_mod);
  return(status);
}

static const char *GetPDFAuthor(const ImageInfo *image_info)
{
  const char
    *option;

  option=GetImageOption(image_info,"pdf:author");
  if (option != (const char *) NULL)
    return(option);
  return(MagickAuthoritativeURL);
}

static const char *GetPDFCreator(const ImageInfo *image_info)
{
  const char
    *option;

  option=GetImageOption(image_info,"pdf:creator");
  if (option != (const char *) NULL)
    return(option);
  return(MagickAuthoritativeURL);
}

static const char *GetPDFProducer(const ImageInfo *image_info)
{
  const char
    *option;

  option=GetImageOption(image_info,"pdf:producer");
  if (option != (const char *) NULL)
    return(option);
  return(MagickAuthoritativeURL);
}

static const char *GetPDFTitle(const ImageInfo *image_info,
  const char *default_title)
{
  const char
    *option;

  option=GetImageOption(image_info,"pdf:title");
  if (option != (const char *) NULL)
    return(option);
  return(default_title);
}

static time_t GetPdfCreationDate(const ImageInfo *image_info,const Image* image)
{
  const char
    *option;

  option=GetImageOption(image_info,"pdf:create-epoch");
  if (option != (const char *) NULL)
    {
      time_t
        epoch;

      epoch=(time_t) StringToDouble(option,(char **) NULL);
      if (epoch > 0)
        return(epoch);
    }
  return(GetBlobProperties(image)->st_ctime);
}

static time_t GetPdfModDate(const ImageInfo *image_info,const Image* image)
{
  const char
    *option;

  option=GetImageOption(image_info,"pdf:modify-epoch");
  if (option != (const char *) NULL)
    {
      time_t
        epoch;

      epoch=(time_t) StringToDouble(option,(char **) NULL);
      if (epoch > 0)
        return(epoch);
    }
  return(GetBlobProperties(image)->st_mtime);
}

static const char *GetPDFSubject(const ImageInfo *image_info)
{
  const char
    *option;

  option=GetImageOption(image_info,"pdf:subject");
  if (option != (const char *) NULL)
    return(option);
  return("");
}

static const char *GetPDFKeywords(const ImageInfo *image_info)
{
  const char
    *option;

  option=GetImageOption(image_info,"pdf:keywords");
  if (option != (const char *) NULL)
    return(option);
  return("");
}

static void WritePDFValue(Image* image,const char *keyword,
  const char *value,const MagickBooleanType is_pdfa)
{
  char
    *escaped;

  size_t
    length;

  ssize_t
    i;

  wchar_t
    *utf16;