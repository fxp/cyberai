// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 17/24

ned int *) AcquireQuantumMemory(length,
    sizeof(*table->levels));
  if (table->levels == (unsigned int *) NULL)
    ThrowFatalException(ResourceLimitFatalError,
      "UnableToAcquireQuantizationTable");
  for (i=0; i < (ssize_t) (table->width*table->height); i++)
  {
    table->levels[i]=(unsigned int) (InterpretLocaleValue(content,&p)/
      table->divisor+0.5);
    while (isspace((int) ((unsigned char) *p)) != 0)
      p++;
    if (*p == ',')
      p++;
    content=p;
  }
  value=InterpretLocaleValue(content,&p);
  (void) value;
  if (p != content)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlInvalidContent","<level> too many values, table \"%s\"",slot);
     quantization_tables=DestroyXMLTree(quantization_tables);
     table=DestroyQuantizationTable(table);
     xml=DestroyString(xml);
     return(table);
   }
  for (j=i; j < 64; j++)
    table->levels[j]=table->levels[j-1];
  quantization_tables=DestroyXMLTree(quantization_tables);
  xml=DestroyString(xml);
  return(table);
}

static void InitializeDestination(j_compress_ptr compress_info)
{
  DestinationManager
    *destination;

  destination=(DestinationManager *) compress_info->dest;
  destination->buffer=(JOCTET *) (*compress_info->mem->alloc_small) (
    (j_common_ptr) compress_info,JPOOL_IMAGE,MagickMinBufferExtent*
    sizeof(JOCTET));
  destination->manager.next_output_byte=destination->buffer;
  destination->manager.free_in_buffer=MagickMinBufferExtent;
}

static void TerminateDestination(j_compress_ptr compress_info)
{
  DestinationManager
    *destination;

  destination=(DestinationManager *) compress_info->dest;
  if ((MagickMinBufferExtent-(int) destination->manager.free_in_buffer) > 0)
    {
      ssize_t
        count;

      count=WriteBlob(destination->image,MagickMinBufferExtent-
        destination->manager.free_in_buffer,destination->buffer);
      if (count != (ssize_t)
          (MagickMinBufferExtent-destination->manager.free_in_buffer))
        ERREXIT(compress_info,JERR_FILE_WRITE);
    }
}

static void WriteProfiles(j_compress_ptr jpeg_info,Image *image,
  ExceptionInfo *exception)
{
  const char
    *name;

  const StringInfo
    *profile;

  MagickBooleanType
    iptc;

  ssize_t
    i;

  size_t
    length,
    tag_length;

  StringInfo
    *custom_profile;

  /*
    Save image profile as a APP marker.
  */
  iptc=MagickFalse;
  custom_profile=AcquireStringInfo(65535L);
  ResetImageProfileIterator(image);
  for (name=GetNextImageProfile(image); name != (const char *) NULL; )
  {
    profile=GetImageProfile(image,name);
    length=GetStringInfoLength(profile);
    if (LocaleNCompare(name,"APP",3) == 0)
      {
        int
          marker;

        marker=APP_MARKER+StringToInteger(name+3);
        for (i=0; i < (ssize_t) length; i+=65533L)
           jpeg_write_marker(jpeg_info,marker,GetStringInfoDatum(profile)+i,
             MagickMin((unsigned int) (length-i),65533));
      }
    else if (LocaleCompare(name,"EXIF") == 0)
      {
        if (length > 65533L)
          (void) ThrowMagickException(exception,GetMagickModule(),CoderWarning,
            "ExifProfileSizeExceedsLimit","`%s'",image->filename);
        else
          jpeg_write_marker(jpeg_info,APP_MARKER+1,GetStringInfoDatum(profile),
            (unsigned int) length);
      }
    else if (LocaleCompare(name,"ICC") == 0)
      {
        unsigned char
          *p;

        tag_length=strlen(ICC_PROFILE);
        p=GetStringInfoDatum(custom_profile);
        (void) memcpy(p,ICC_PROFILE,tag_length);
        p[tag_length]='\0';
        for (i=0; i < (ssize_t) GetStringInfoLength(profile); i+=65519L)
        {
          length=(size_t) MagickMin((ssize_t) GetStringInfoLength(profile)-i,
            65519L);
          p[12]=(unsigned char) ((i/65519L)+1);
          p[13]=(unsigned char) (GetStringInfoLength(profile)/65519L+1);
          (void) memcpy(p+tag_length+3,GetStringInfoDatum(profile)+i,
            length);
          jpeg_write_marker(jpeg_info,ICC_MARKER,GetStringInfoDatum(
            custom_profile),(unsigned int) (length+tag_length+3));
        }
      }
    else if (((LocaleCompare(name,"IPTC") == 0) ||
        (LocaleCompare(name,"8BIM") == 0)) && (iptc == MagickFalse))
      {
        size_t
          roundup;

        unsigned char
          *p;