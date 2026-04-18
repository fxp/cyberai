// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 10/30



  assert(source != (const char *) NULL);
  length=0;
  for (p=source; *p != '\0'; p++)
  {
    if ((*p == '\\') || (*p == '(') || (*p == ')'))
      {
        if (~length < 1)
          ThrowFatalException(ResourceLimitFatalError,"UnableToEscapeString");
        length++;
      }
    length++;
  }
  destination=(char *) NULL;
  if (~length >= (MagickPathExtent-1))
    destination=(char *) AcquireQuantumMemory(length+MagickPathExtent,
      sizeof(*destination));
  if (destination == (char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"UnableToEscapeString");
  *destination='\0';
  q=destination;
  for (p=source; *p != '\0'; p++)
  {
    if ((*p == '\\') || (*p == '(') || (*p == ')'))
      *q++='\\';
    *q++=(*p);
  }
  *q='\0';
  return(destination);
}

static size_t UTF8ToUTF16(const unsigned char *utf8,wchar_t *utf16)
{
  const unsigned char
    *p;

  if (utf16 != (wchar_t *) NULL)
    {
      wchar_t
        *q;

      wchar_t
        c;

      /*
        Convert UTF-8 to UTF-16.
      */
      q=utf16;
      for (p=utf8; *p != '\0'; p++)
      {
        if ((*p & 0x80) == 0)
          *q=(*p);
        else
          if ((*p & 0xE0) == 0xC0)
            {
              c=(*p);
              *q=(c & 0x1F) << 6;
              p++;
              if ((*p & 0xC0) != 0x80)
                return(0);
              *q|=(*p & 0x3F);
            }
          else
            if ((*p & 0xF0) == 0xE0)
              {
                c=(*p);
                *q=c << 12;
                p++;
                if ((*p & 0xC0) != 0x80)
                  return(0);
                c=(*p);
                *q|=(c & 0x3F) << 6;
                p++;
                if ((*p & 0xC0) != 0x80)
                  return(0);
                *q|=(*p & 0x3F);
              }
            else
              return(0);
        q++;
      }
      *q++=(wchar_t) '\0';
      return((size_t) (q-utf16));
    }
  /*
    Compute UTF-16 string length.
  */
  for (p=utf8; *p != '\0'; p++)
  {
    if ((*p & 0x80) == 0)
      ;
    else
      if ((*p & 0xE0) == 0xC0)
        {
          p++;
          if ((*p & 0xC0) != 0x80)
            return(0);
        }
      else
        if ((*p & 0xF0) == 0xE0)
          {
            p++;
            if ((*p & 0xC0) != 0x80)
              return(0);
            p++;
            if ((*p & 0xC0) != 0x80)
              return(0);
         }
       else
         return(0);
  }
  return((size_t) (p-utf8));
}

static wchar_t *ConvertUTF8ToUTF16(const unsigned char *source,size_t *length)
{
  wchar_t
    *utf16;

  *length=UTF8ToUTF16(source,(wchar_t *) NULL);
  if (*length == 0)
    {
      ssize_t
        i;

      /*
        Not UTF-8, just copy.
      */
      *length=strlen((const char *) source);
      utf16=(wchar_t *) AcquireQuantumMemory(*length+1,sizeof(*utf16));
      if (utf16 == (wchar_t *) NULL)
        return((wchar_t *) NULL);
      for (i=0; i <= (ssize_t) *length; i++)
        utf16[i]=source[i];
      return(utf16);
    }
  utf16=(wchar_t *) AcquireQuantumMemory(*length+1,sizeof(*utf16));
  if (utf16 == (wchar_t *) NULL)
    return((wchar_t *) NULL);
  *length=UTF8ToUTF16(source,utf16);
  return(utf16);
}

static MagickBooleanType Huffman2DEncodeImage(const ImageInfo *image_info,
  Image *image,Image *inject_image,ExceptionInfo *exception)
{
  Image
    *group4_image;

  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  size_t
    length;

  unsigned char
    *group4;

  group4_image=CloneImage(inject_image,0,0,MagickTrue,exception);
  if (group4_image == (Image *) NULL)
    return(MagickFalse);
  status=MagickTrue;
  write_info=CloneImageInfo(image_info);
  (void) CopyMagickString(write_info->filename,"GROUP4:",MagickPathExtent);
  (void) CopyMagickString(write_info->magick,"GROUP4",MagickPathExtent);
  (void) SetImageArtifact(group4_image,"tiff:photometric","min-is-white");
  group4=(unsigned char *) ImageToBlob(write_info,group4_image,&length,
    exception);
  group4_image=DestroyImage(group4_image);
  write_info=DestroyImageInfo(write_info);
  if (group4 == (unsigned char *) NULL)
    return(MagickFalse);
  if (WriteBlob(image,length,group4) != (ssize_t) length)
    status=MagickFalse;
  group4=(unsigned char *) RelinquishMagickMemory(group4);
  return(status);
}

static MagickBooleanType WritePOCKETMODImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
#define PocketPageOrder  "1,2,3,4,0,7,6,5"

  const Image
    *next;

  Image
    *pages,
    *pocket_mod;

  MagickBooleanType
    status;

  ssize_t
    i;