// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 6/34



  status=MagickTrue;
  if ((TIFFGetField(tiff,TIFFTAG_ARTIST,&text,sans) == 1) &&
      (text != (char *) NULL))
    status=SetImageProperty(image,"tiff:artist",text,exception);
  if ((TIFFGetField(tiff,TIFFTAG_COPYRIGHT,&text,sans) == 1) &&
      (text != (char *) NULL))
    status=SetImageProperty(image,"tiff:copyright",text,exception);
  if ((TIFFGetField(tiff,TIFFTAG_DATETIME,&text,sans) == 1) &&
      (text != (char *) NULL))
    status=SetImageProperty(image,"tiff:timestamp",text,exception);
  if ((TIFFGetField(tiff,TIFFTAG_DOCUMENTNAME,&text,sans) == 1) &&
      (text != (char *) NULL))
    status=SetImageProperty(image,"tiff:document",text,exception);
  if ((TIFFGetField(tiff,TIFFTAG_HOSTCOMPUTER,&text,sans) == 1) &&
      (text != (char *) NULL))
    status=SetImageProperty(image,"tiff:hostcomputer",text,exception);
  if ((TIFFGetField(tiff,TIFFTAG_IMAGEDESCRIPTION,&text,sans) == 1) &&
      (text != (char *) NULL))
    status=SetImageProperty(image,"comment",text,exception);
  if ((TIFFGetField(tiff,TIFFTAG_MAKE,&text,sans) == 1) &&
      (text != (char *) NULL))
    status=SetImageProperty(image,"tiff:make",text,exception);
  if ((TIFFGetField(tiff,TIFFTAG_MODEL,&text,sans) == 1) &&
      (text != (char *) NULL))
    status=SetImageProperty(image,"tiff:model",text,exception);
  if ((TIFFGetField(tiff,TIFFTAG_OPIIMAGEID,&count,&text,sans) == 1) &&
      (count != 0) && (text != (char *) NULL))
    {
      if (count >= MagickPathExtent)
        count=MagickPathExtent-1;
      (void) CopyMagickString(message,text,count+1);
      status=SetImageProperty(image,"tiff:image-id",message,exception);
    }
  if ((TIFFGetField(tiff,TIFFTAG_PAGENAME,&text,sans) == 1) &&
      (count != 0) && (text != (char *) NULL))
    status=SetImageProperty(image,"label",text,exception);
  if ((TIFFGetField(tiff,TIFFTAG_SOFTWARE,&text) == 1) &&
      (text != (char *) NULL))
    status=SetImageProperty(image,"tiff:software",text,exception);
  if ((TIFFGetField(tiff,33423,&count,&text,sans) == 1) &&
      (count != 0) && (text != (char *) NULL))
    {
      if (count >= MagickPathExtent)
        count=MagickPathExtent-1;
      (void) CopyMagickString(message,text,count+1);
      status=SetImageProperty(image,"tiff:kodak-33423",message,exception);
    }
  if ((TIFFGetField(tiff,36867,&count,&text,sans) == 1) &&
      (count != 0) && (text != (char *) NULL))
    {
      if (count >= MagickPathExtent)
        count=MagickPathExtent-1;
      (void) CopyMagickString(message,text,count+1);
      status=SetImageProperty(image,"tiff:kodak-36867",message,exception);
    }
  if (TIFFGetField(tiff,TIFFTAG_SUBFILETYPE,&type,sans) == 1)
    switch (type)
    {
      case 0x01:
      {
        status=SetImageProperty(image,"tiff:subfiletype","REDUCEDIMAGE",
          exception);
        break;
      }
      case 0x02:
      {
        status=SetImageProperty(image,"tiff:subfiletype","PAGE",exception);
        break;
      }
      case 0x04:
      {
        status=SetImageProperty(image,"tiff:subfiletype","MASK",exception);
        break;
      }
      default:
        break;
    }
  return(status);
}

static void TIFFSetImageProperties(TIFF *tiff,Image *image,const char *tag,
  ExceptionInfo *exception)
{
  char
    buffer[MagickPathExtent],
    filename[MagickPathExtent];

  FILE
    *file;

  int
    unique_file;

  /*
    Set EXIF or GPS image properties.
  */
  unique_file=AcquireUniqueFileResource(filename);
  file=(FILE *) NULL;
  if (unique_file != -1)
    file=fdopen(unique_file,"rb+");
  if ((unique_file == -1) || (file == (FILE *) NULL))
    {
      (void) RelinquishUniqueFileResource(filename);
      (void) ThrowMagickException(exception,GetMagickModule(),WandError,
        "UnableToCreateTemporaryFile","`%s'",filename);
      return;
    }
  TIFFPrintDirectory(tiff,file,0);
  (void) fseek(file,0,SEEK_SET);
  while (fgets(buffer,(int) sizeof(buffer),file) != NULL)
  {
    char
      *p,
      property[MagickPathExtent],
      value[MagickPathExtent];

    (void) StripMagickString(buffer);
    p=strchr(buffer,':');
    if (p == (char *) NULL)
      continue;
    *p='\0';
    (void) FormatLocaleString(property,MagickPathExtent,"%s%s",tag,buffer);
    (void) FormatLocaleString(value,MagickPathExtent,"%s",p+1);
    (void) StripMagickString(value);
    (void) SetImageProperty(image,property,value,exception);
  }
  (void) fclose(file);
  (void) RelinquishUniqueFileResource(filename);
}

static void TIFFGetEXIFProperties(TIFF *tiff,Image *image,
  const ImageInfo* image_info,ExceptionInfo *exception)
{
  const char
    *option;

  tdir_t
    directory;

#if defined(TIFF_VERSION_BIG)
  uint64
#else
  uint32
#endif
    offset;