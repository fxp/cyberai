// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 5/24



  /*
    Determine length of binary data stored here.
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
  /*
    Validate that this was written as a Photoshop resource format slug.
  */
  for (i=0; i < 10; i++)
    magick[i]=(char) GetCharacter(jpeg_info);
  magick[10]='\0';
  length-=10;
  if (length <= 10)
    return(TRUE);
  if (LocaleCompare(magick,"Photoshop ") != 0)
    {
      /*
        Not a IPTC profile, return.
      */
      for (i=0; i < (ssize_t) length; i++)
        if (GetCharacter(jpeg_info) == EOF)
          break;
      return(TRUE);
    }
  /*
    Remove the version number.
  */
  if (length <= 15)
    return(TRUE);
  for (i=0; i < 4; i++)
    if (GetCharacter(jpeg_info) == EOF)
      break;
  if (length <= 11)
    return(TRUE);
  length-=4;
  status=ReadProfilePayload(jpeg_info,IPTC_INDEX,length);
  if (status == MagickFalse)
    return(FALSE);
  client_info=(JPEGClientInfo *) jpeg_info->client_data;
  image=client_info->image;
  exception=client_info->exception;
  status=SetImageProfile(image,"8bim",client_info->profiles[IPTC_INDEX],
    exception);
  return(status == MagickFalse ? FALSE : TRUE);
}

static boolean ReadAPPProfiles(j_decompress_ptr jpeg_info)
{
  ExceptionInfo
    *exception;

  Image
    *image;

  int
    marker;

  JPEGClientInfo
    *client_info;

  MagickBooleanType
    status;

  size_t
    length,
    offset = 0;

  StringInfo
    *profile;

  unsigned char
    *p;

  /*
    Read generic profile.
  */
  GetProfileLength(jpeg_info,length);
  if (length <= 2)
    return(TRUE);
  length-=2;
  marker=jpeg_info->unread_marker-JPEG_APP0;
  client_info=(JPEGClientInfo *) jpeg_info->client_data;
  image=client_info->image;
  exception=client_info->exception;
  if (client_info->profiles[marker] != (StringInfo *) NULL)
    offset=GetStringInfoLength(client_info->profiles[marker]);
  status=ReadProfilePayload(jpeg_info,marker,length);
  if (status == MagickFalse)
    return(FALSE);
  if (marker != 1)
    return(TRUE);
  p=GetStringInfoDatum(client_info->profiles[marker])+offset;
  if ((length > strlen(xmp_namespace)) &&
      (LocaleNCompare((char *) p,xmp_namespace,strlen(xmp_namespace)-1) == 0))
    {
      size_t
        i;

      /*
        Extract XMP profile minus the namespace header.
      */
      for (i=0; i < length; i++)
      {
        if (*p == '\0')
          break;
        p++;
      }
      if (i != length)
        {
          profile=AcquireProfileStringInfo("xmp",length,exception);
          if (profile != (StringInfo*) NULL)
            {
              (void) memcpy(GetStringInfoDatum(profile),p+1,length-i-1);
              SetStringInfoLength(profile,length-i-1);
              status=SetImageProfilePrivate(image,profile,exception);
            }
          client_info->profiles[marker]=DestroyStringInfo(
            client_info->profiles[marker]);
        }
    }
  else
    {
      status=SetImageProfile(image,"app1",client_info->profiles[marker],
        exception);
      client_info->profiles[marker]=DestroyStringInfo(
        client_info->profiles[marker]);
    }
  return(status == MagickFalse ? FALSE : TRUE);
}

static void SkipInputData(j_decompress_ptr compress_info,long number_bytes)
{
  SourceManager
    *source;

  if (number_bytes <= 0)
    return;
  source=(SourceManager *) compress_info->src;
  while (number_bytes > (long) source->manager.bytes_in_buffer)
  {
    number_bytes-=(long) source->manager.bytes_in_buffer;
    (void) FillInputBuffer(compress_info);
  }
  source->manager.next_input_byte+=number_bytes;
  source->manager.bytes_in_buffer-=(size_t) number_bytes;
}

static void TerminateSource(j_decompress_ptr compress_info)
{
  (void) compress_info;
}

static void JPEGSourceManager(j_decompress_ptr compress_info,Image *image)
{
  SourceManager
    *source;

  compress_info->src=(struct jpeg_source_mgr *)
    (*compress_info->mem->alloc_small) ((j_common_ptr) compress_info,
    JPOOL_IMAGE,sizeof(SourceManager));
  source=(SourceManager *) compress_info->src;
  source->buffer=(JOCTET *) (*compress_info->mem->alloc_small) ((j_common_ptr)
    compress_info,JPOOL_IMAGE,MagickMinBufferExtent*sizeof(JOCTET));
  source=(SourceManager *) compress_info->src;
  source->manager.init_source=InitializeSource;
  source->manager.fill_input_buffer=FillInputBuffer;
  source->manager.skip_input_data=SkipInputData;
  source->manager.resync_to_restart=jpeg_resync_to_restart;
  source->manager.term_source=TerminateSource;
  source->manager.bytes_in_buffer=0;
  source->manager.next_input_byte=NULL;
  source->image=image;
}