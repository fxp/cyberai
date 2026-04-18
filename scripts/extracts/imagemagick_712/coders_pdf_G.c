// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 7/30



      (void) FormatLocaleString(pages,MagickPathExtent,"-dFirstPage=%.20g "
        "-dLastPage=%.20g",(double) read_info->scene+1,(double)
        (read_info->scene+read_info->number_scenes));
      (void) ConcatenateMagickString(options,pages,MagickPathExtent);
      read_info->number_scenes=0;
      if (read_info->scenes != (char *) NULL)
        *read_info->scenes='\0';
    }
  (void) CopyMagickString(filename,read_info->filename,MagickPathExtent);
  (void) AcquireUniqueFilename(filename);
  (void) RelinquishUniqueFileResource(filename);
  (void) ConcatenateMagickString(filename,"%d",MagickPathExtent);
  (void) FormatLocaleString(command,MagickPathExtent,
    GetDelegateCommands(delegate_info),
    read_info->antialias != MagickFalse ? 4 : 1,
    read_info->antialias != MagickFalse ? 4 : 1,density,options,filename,
    postscript_filename,input_filename);
  options=DestroyString(options);
  density=DestroyString(density);
  *message='\0';
  status=InvokeGhostscriptDelegate(read_info->verbose,command,message,
    exception);
  (void) RelinquishUniqueFileResource(postscript_filename);
  (void) RelinquishUniqueFileResource(input_filename);
  pdf_image=(Image *) NULL;
  if (status == MagickFalse)
    for (i=1; ; i++)
    {
      (void) InterpretImageFilename(image_info,image,filename,(int) i,
        read_info->filename,exception);
      if (IsGhostscriptRendered(read_info->filename) == MagickFalse)
        break;
      (void) RelinquishUniqueFileResource(read_info->filename);
    }
  else
    {
      next=(Image *) NULL;
      for (i=1; ; i++)
      {
        (void) InterpretImageFilename(image_info,image,filename,(int) i,
          read_info->filename,exception);
        if (IsGhostscriptRendered(read_info->filename) == MagickFalse)
          break;
        read_info->blob=NULL;
        read_info->length=0;
        next=ReadImage(read_info,exception);
        (void) RelinquishUniqueFileResource(read_info->filename);
        if (next == (Image *) NULL)
          break;
        AppendImageToList(&pdf_image,next);
      }
      /* Clean up remaining files */
      if (next == (Image *) NULL)
        {
          ssize_t
            j;

          for (j=i+1; ; j++)
            {
              (void) InterpretImageFilename(image_info,image,filename,(int) j,
                read_info->filename,exception);
              if (IsGhostscriptRendered(read_info->filename) == MagickFalse)
                break;
              (void) RelinquishUniqueFileResource(read_info->filename);
            }
        }
    }
  read_info=DestroyImageInfo(read_info);
  if (pdf_image == (Image *) NULL)
    {
      if (*message != '\0')
        (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
          "PDFDelegateFailed","`%s'",message);
      CleanupPDFInfo(&pdf_info);
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  if (LocaleCompare(pdf_image->magick,"BMP") == 0)
    {
      Image
        *cmyk_image;

      cmyk_image=ConsolidateCMYKImages(pdf_image,exception);
      if (cmyk_image != (Image *) NULL)
        {
          pdf_image=DestroyImageList(pdf_image);
          pdf_image=cmyk_image;
        }
    }
  if (pdf_info.xmp_profile != (StringInfo *) NULL)
    {
      char
        *profile;

      profile=(char *) GetStringInfoDatum(pdf_info.xmp_profile);
      if (strstr(profile,"Adobe Illustrator") != (char *) NULL)
        (void) CopyMagickString(image->magick,"AI",MagickPathExtent);
      (void) SetImageProfilePrivate(image,pdf_info.xmp_profile,exception);
      pdf_info.xmp_profile=(StringInfo *) NULL;
    }
  if (image_info->number_scenes != 0)
    {
      Image
        *clone_image;