// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 4/24



  client_info=(JPEGClientInfo *) jpeg_info->client_data;
  exception=client_info->exception;
  image=client_info->image;
  if ((index < 0) || (index > MaxJPEGProfiles))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
        "TooManyProfiles","`%s'",image->filename);
      return(MagickFalse);
    }
  if (client_info->profiles[index] == (StringInfo *) NULL)
    client_info->profiles[index]=AcquireStringInfo(length);
  else
    {
      offset=GetStringInfoLength(client_info->profiles[index]);
      SetStringInfoLength(client_info->profiles[index],offset+length);
    }
  p=GetStringInfoDatum(client_info->profiles[index])+offset;
  for (i=0; i < (ssize_t) length; i++)
  {
    int
      c;

    c=GetCharacter(jpeg_info);
    if (c == EOF)
      break;
    *p++=(unsigned char) c;
  }
  if (i != (ssize_t) length)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        CorruptImageError,"InsufficientImageDataInFile","`%s'",image->filename);
      return(MagickFalse);
    }
  *p='\0';
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "Profile[%.20g]: %.20g bytes",(double) index,(double) length);
  return(MagickTrue);
}

static boolean ReadComment(j_decompress_ptr jpeg_info)
{
#define GetProfileLength(jpeg_info,length) \
do \
{ \
  int \
    c; \
 \
  if (((c=GetCharacter(jpeg_info)) == EOF) || (c < 0)) \
    length=0; \
  else \
    { \
      length=(size_t) (256*c); \
      if (((c=GetCharacter(jpeg_info)) == EOF) || (c < 0)) \
        length=0; \
      else \
        length+=(unsigned char) c; \
    } \
} while(0)

  ExceptionInfo
    *exception;

  Image
    *image;

  JPEGClientInfo
    *client_info;

  MagickBooleanType
    status;

  size_t
    length;

  /*
    Determine length of comment.
  */
  GetProfileLength(jpeg_info,length);
  if (length <= 2)
    return(TRUE);
  length-=2;
  status=ReadProfilePayload(jpeg_info,COMMENT_INDEX,length);
  if (status == MagickFalse)
    return(FALSE);
  client_info=(JPEGClientInfo *) jpeg_info->client_data;
  image=client_info->image;
  exception=client_info->exception;
  status=SetImageProperty(image,"comment",(const char *) GetStringInfoDatum(
    client_info->profiles[COMMENT_INDEX]),exception);
  return(status == MagickFalse ? FALSE : TRUE);
}

static boolean ReadICCProfile(j_decompress_ptr jpeg_info)
{
  char
    magick[13];

  ExceptionInfo
    *exception;

  Image
    *image;

  JPEGClientInfo
    *client_info;

  MagickBooleanType
    status;

  size_t
    length;

  ssize_t
    i;

  /*
    Read color profile.
  */
  GetProfileLength(jpeg_info,length);
  if (length <= 2)
    return(TRUE);
  length-=2;
  if (length <= 14)
    {
      while (length-- > 0)
        if (GetCharacter(jpeg_info) == EOF)
          break;
      return(TRUE);
    }
  for (i=0; i < 12; i++)
    magick[i]=(char) GetCharacter(jpeg_info);
  magick[i]='\0';
  if (LocaleCompare(magick,ICC_PROFILE) != 0)
    {
      if (LocaleCompare(magick,"MPF") == 0)
        {
          /*
            Multi-picture support (Future).
          */
          status=ReadProfilePayload(jpeg_info,ICC_INDEX,length-12);
          if (status == MagickFalse)
            return(FALSE);
          client_info=(JPEGClientInfo *) jpeg_info->client_data;
          status=SetImageProfile(client_info->image,"MPF",
            client_info->profiles[ICC_INDEX],client_info->exception);
          client_info->profiles[ICC_INDEX]=DestroyStringInfo(
            client_info->profiles[ICC_INDEX]);
          return(TRUE);
        }
      /*
        Not a ICC profile, return.
      */
      for (i=0; i < (ssize_t) (length-12); i++)
        if (GetCharacter(jpeg_info) == EOF)
          break;
      return(TRUE);
    }
  (void) GetCharacter(jpeg_info);  /* id */
  (void) GetCharacter(jpeg_info);  /* markers */
  length-=14;
  status=ReadProfilePayload(jpeg_info,ICC_INDEX,length);
  if (status == MagickFalse)
    return(FALSE);
  client_info=(JPEGClientInfo *) jpeg_info->client_data;
  image=client_info->image;
  exception=client_info->exception;
  status=SetImageProfile(image,"icc",client_info->profiles[ICC_INDEX],
    exception);
  return(status == MagickFalse ? FALSE : TRUE);
}

static boolean ReadIPTCProfile(j_decompress_ptr jpeg_info)
{
  char
    magick[MagickPathExtent];

  ExceptionInfo
    *exception;

  Image
    *image;

  JPEGClientInfo
    *client_info;

  MagickBooleanType
    status;

  size_t
    length;

  ssize_t
    i;