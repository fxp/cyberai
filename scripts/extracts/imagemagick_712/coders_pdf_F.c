// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 6/30



      swap=page.width;
      page.width=page.height;
      page.height=swap;
    }
  if (IssRGBCompatibleColorspace(image_info->colorspace) != MagickFalse)
    pdf_info.cmyk=MagickFalse;
  /*
    Create Ghostscript control file.
  */
  file=AcquireUniqueFileResource(postscript_filename);
  if (file == -1)
    {
      (void) RelinquishUniqueFileResource(input_filename);
      ThrowFileException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        image_info->filename);
      CleanupPDFInfo(&pdf_info);
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  if (write(file," ",1) != 1)
    {
      file=close_utf8(file)-1;
      (void) RelinquishUniqueFileResource(input_filename);
      (void) RelinquishUniqueFileResource(postscript_filename);
      CleanupPDFInfo(&pdf_info);
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  file=close_utf8(file)-1;
  /*
    Render Postscript with the Ghostscript delegate.
  */
  if (image_info->monochrome != MagickFalse)
    delegate_info=GetDelegateInfo("ps:mono",(char *) NULL,exception);
  else
     if (pdf_info.cmyk != MagickFalse)
       delegate_info=GetDelegateInfo("ps:cmyk",(char *) NULL,exception);
     else
       delegate_info=GetDelegateInfo("ps:alpha",(char *) NULL,exception);
  if (delegate_info == (const DelegateInfo *) NULL)
    {
      (void) RelinquishUniqueFileResource(input_filename);
      (void) RelinquishUniqueFileResource(postscript_filename);
      CleanupPDFInfo(&pdf_info);
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  density=AcquireString("");
  options=AcquireString("");
  (void) FormatLocaleString(density,MagickPathExtent,"%gx%g",
    image->resolution.x,image->resolution.y);
  if (image_info->ping != MagickFalse)
    (void) FormatLocaleString(density,MagickPathExtent,"2.0x2.0");
  else
    if ((image_info->page != (char *) NULL) || (fitPage != MagickFalse))
      (void) FormatLocaleString(options,MagickPathExtent,"-g%.20gx%.20g ",
        (double) page.width,(double) page.height);
  option=GetImageOption(image_info,"pdf:printed");
  if (IsStringTrue(option) != MagickFalse)
    (void) ConcatenateMagickString(options,"-dPrinted=true ",MagickPathExtent);
  else
    (void) ConcatenateMagickString(options,"-dPrinted=false ",MagickPathExtent);
  if (fitPage != MagickFalse)
    (void) ConcatenateMagickString(options,"-dPSFitPage ",MagickPathExtent);
  if (pdf_info.cropbox != MagickFalse)
    (void) ConcatenateMagickString(options,"-dUseCropBox ",MagickPathExtent);
  if (pdf_info.trimbox != MagickFalse)
    (void) ConcatenateMagickString(options,"-dUseTrimBox ",MagickPathExtent);
  option=GetImageOption(image_info,"pdf:stop-on-error");
  if (IsStringTrue(option) != MagickFalse)
    (void) ConcatenateMagickString(options,"-dPDFSTOPONERROR ",
      MagickPathExtent);
  option=GetImageOption(image_info,"pdf:interpolate");
  if (IsStringTrue(option) != MagickFalse)
    (void) ConcatenateMagickString(options,"-dInterpolateControl=-1 ",
      MagickPathExtent);
  option=GetImageOption(image_info,"pdf:hide-annotations");
  if (IsStringTrue(option) != MagickFalse)
    (void) ConcatenateMagickString(options,"-dShowAnnots=false ",
      MagickPathExtent);
  option=GetImageOption(image_info,"authenticate");
  if (option != (char *) NULL)
    {
      char
        passphrase[MagickPathExtent];

      FormatSanitizedDelegateOption(passphrase,MagickPathExtent,
        "\"-sPDFPassword=%s\" ","-sPDFPassword='%s' ",option);
      (void) ConcatenateMagickString(options,passphrase,MagickPathExtent);
    }
  read_info=CloneImageInfo(image_info);
  *read_info->magick='\0';
  if (read_info->number_scenes != 0)
    {
      char
        pages[MagickPathExtent];