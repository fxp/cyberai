// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 18/24



        iptc=MagickTrue;
        p=GetStringInfoDatum(custom_profile);
        for (i=0; i < (ssize_t) GetStringInfoLength(profile); i+=65500L)
        {
          length=(size_t) MagickMin((ssize_t) GetStringInfoLength(profile)-i,
            65500L);
          roundup=(size_t) (length & 0x01);
          if (LocaleNCompare((char *) GetStringInfoDatum(profile),"8BIM",4) == 0)
            {
              (void) memcpy(p,"Photoshop 3.0 ",14);
              tag_length=14;
            }
          else
            {
              (void) memcpy(p,"Photoshop 3.0 8BIM\04\04\0\0\0\0",24);
              tag_length=26;
              p[24]=(unsigned char) (length >> 8);
              p[25]=(unsigned char) (length & 0xff);
            }
          p[13]=0x00;
          (void) memcpy(p+tag_length,GetStringInfoDatum(profile)+i,length);
          if (roundup != 0)
            p[length+tag_length]='\0';
          jpeg_write_marker(jpeg_info,IPTC_MARKER,GetStringInfoDatum(
            custom_profile),(unsigned int) (length+tag_length+roundup));
        }
      }
    else if (LocaleCompare(name,"XMP") == 0)
      {
        StringInfo
          *xmp_profile;

        /*
          Add namespace to XMP profile.
        */
        xmp_profile=StringToStringInfo(xmp_namespace);
        SetStringInfoLength(xmp_profile,strlen(xmp_namespace)+1);
        ConcatenateStringInfo(xmp_profile,profile);
        GetStringInfoDatum(xmp_profile)[strlen(xmp_namespace)]='\0';
        length=GetStringInfoLength(xmp_profile);
        for (i=0; i < (ssize_t) length; i+=65533L)
          jpeg_write_marker(jpeg_info,APP_MARKER+1,GetStringInfoDatum(
            xmp_profile)+i,MagickMin((unsigned int) (length-i),65533));
        xmp_profile=DestroyStringInfo(xmp_profile);
      }
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "%s profile: %.20g bytes",name,(double) GetStringInfoLength(profile));
    name=GetNextImageProfile(image);
  }
  custom_profile=DestroyStringInfo(custom_profile);
}

static void JPEGDestinationManager(j_compress_ptr compress_info,Image * image)
{
  DestinationManager
    *destination;

  compress_info->dest=(struct jpeg_destination_mgr *) (*
    compress_info->mem->alloc_small) ((j_common_ptr) compress_info,JPOOL_IMAGE,
    sizeof(DestinationManager));
  destination=(DestinationManager *) compress_info->dest;
  destination->manager.init_destination=InitializeDestination;
  destination->manager.empty_output_buffer=EmptyOutputBuffer;
  destination->manager.term_destination=TerminateDestination;
  destination->image=image;
}

static char **SamplingFactorToList(const char *text)
{
  char
    *q,
    **textlist;

  const char
    *p;

  ssize_t
    i;

  /*
    Convert string to an ASCII list.
  */
  if (text == (char *) NULL)
    return((char **) NULL);
  textlist=(char **) AcquireQuantumMemory((size_t) MAX_COMPONENTS,
    sizeof(*textlist));
  if (textlist == (char **) NULL)
    ThrowFatalException(ResourceLimitFatalError,"UnableToConvertText");
  p=text;
  for (i=0; i < (ssize_t) MAX_COMPONENTS; i++)
  {
    for (q=(char *) p; *q != '\0'; q++)
      if (*q == ',')
        break;
    textlist[i]=(char *) AcquireQuantumMemory((size_t) (q-p)+MagickPathExtent,
      sizeof(*textlist[i]));
    if (textlist[i] == (char *) NULL)
      ThrowFatalException(ResourceLimitFatalError,"UnableToConvertText");
    (void) CopyMagickString(textlist[i],p,(size_t) (q-p+1));
    if (*q == '\r')
      q++;
    if (*q == '\0')
      break;
    p=q+1;
  }
  for (i++; i < (ssize_t) MAX_COMPONENTS; i++)
    textlist[i]=ConstantString("1x1");
  return(textlist);
}

static inline void JPEGSetSample(const struct jpeg_compress_struct *jpeg_info,
  const Quantum pixel,QuantumAny range,JSAMPLE *q)
{
  if (jpeg_info->data_precision == 8)
    *q=(JSAMPLE) ScaleQuantumToChar(pixel);
  else if (jpeg_info->data_precision == 16)
    (*(unsigned short *) q)=(unsigned short) ScaleQuantumToShort(pixel);
  else if (jpeg_info->data_precision < 8)
    *q=(JSAMPLE) ScaleQuantumToAny(pixel,range);
  else
    (*(unsigned short *) q)=(unsigned short) ScaleQuantumToAny(pixel,range);
}

static MagickBooleanType WriteJPEGImage_(const ImageInfo *image_info,
  Image *myImage,struct jpeg_compress_struct *jpeg_info,
  ExceptionInfo *exception)
{
#define ThrowJPEGWriterException(exception,message) \
{ \
  if (client_info != (JPEGClientInfo *) NULL) \
    client_info=(JPEGClientInfo *) RelinquishMagickMemory(client_info); \
  ThrowWriterException((exception),(message)); \
}

  const char
    *dct_method,
    *option,
    *sampling_factor,
    *value;

  Image
    *volatile image = (Image *) NULL,
    *volatile jps_image = (Image *) NULL,
    *volatile volatile_image = (Image *) NULL;

  int
    colorspace,
    quality;

  JPEGClientInfo
    *client_info = (JPEGClientInfo *) NULL;

  JSAMPLE
    *volatile jpeg_pixels,
    *q;

  MagickBooleanType
    status;

  MemoryInfo
    *memory_info;

  QuantumAny
    range;