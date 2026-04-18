// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 26/34



  base_image=CloneImage(image,0,0,MagickFalse,exception);
  if (base_image == (Image *) NULL)
    return(MagickTrue);
  clone_info=CloneImageInfo(image_info);
  if (clone_info == (ImageInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  profile.offset=0;
  profile.quantum=MagickMinBlobExtent;
  layers=AcquireProfileStringInfo("tiff:37724",profile.quantum,
    exception);
  if (layers == (StringInfo *) NULL)
    {
      base_image=DestroyImage(base_image);
      clone_info=DestroyImageInfo(clone_info);
      return(MagickTrue);
    }
  profile.data=layers;
  profile.extent=layers->length;
  custom_stream=TIFFAcquireCustomStreamForWriting(&profile,exception);
  if (custom_stream == (CustomStreamInfo *) NULL)
    {
      base_image=DestroyImage(base_image);
      clone_info=DestroyImageInfo(clone_info);
      layers=DestroyStringInfo(layers);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  blob=CloneBlobInfo((BlobInfo *) NULL);
  if (blob == (BlobInfo *) NULL)
    {
      base_image=DestroyImage(base_image);
      clone_info=DestroyImageInfo(clone_info);
      layers=DestroyStringInfo(layers);
      custom_stream=DestroyCustomStreamInfo(custom_stream);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  DestroyBlob(base_image);
  base_image->blob=blob;
  next=base_image;
  while (next != (Image *) NULL)
    next=SyncNextImageInList(next);
  AttachCustomStream(base_image->blob,custom_stream);
  InitPSDInfo(image,&info);
  base_image->endian=endian;
  WriteBlobString(base_image,"Adobe Photoshop Document Data Block");
  WriteBlobByte(base_image,0);
  WriteBlobString(base_image,base_image->endian == LSBEndian ? "MIB8ryaL" :
    "8BIMLayr");
  status=WritePSDLayers(base_image,clone_info,&info,exception);
  if (status != MagickFalse)
    {
      SetStringInfoLength(layers,(size_t) profile.offset);
      (void) SetImageProfilePrivate(image,layers,exception);
    }
  else
    layers=DestroyStringInfo(layers);
  next=base_image;
  while (next != (Image *) NULL)
  {
    CloseBlob(next);
    next=next->next;
  }
  clone_info=DestroyImageInfo(clone_info);
  custom_stream=DestroyCustomStreamInfo(custom_stream);
  return(status);
}

static void TIFFSetProfiles(TIFF *tiff,Image *image)
{
  const char
    *name;

  const StringInfo
    *profile;

  if (image->profiles == (void *) NULL)
    return;
  ResetImageProfileIterator(image);
  for (name=GetNextImageProfile(image); name != (const char *) NULL; )
  {
    profile=GetImageProfile(image,name);
    if (GetStringInfoLength(profile) == 0)
      {
        name=GetNextImageProfile(image);
        continue;
      }
#if defined(TIFFTAG_XMLPACKET)
    if (LocaleCompare(name,"xmp") == 0)
      (void) TIFFSetField(tiff,TIFFTAG_XMLPACKET,(uint32) GetStringInfoLength(
        profile),GetStringInfoDatum(profile));
#endif
#if defined(TIFFTAG_ICCPROFILE)
    if (LocaleCompare(name,"icc") == 0)
      (void) TIFFSetField(tiff,TIFFTAG_ICCPROFILE,(uint32) GetStringInfoLength(
        profile),GetStringInfoDatum(profile));
#endif
    if (LocaleCompare(name,"iptc") == 0)
      {
        const TIFFField
          *field;

        size_t
          length;

        StringInfo
          *iptc_profile;

        iptc_profile=CloneStringInfo(profile);
        length=GetStringInfoLength(profile)+4-(GetStringInfoLength(profile) &
          0x03);
        SetStringInfoLength(iptc_profile,length);
        field=TIFFFieldWithTag(tiff,TIFFTAG_RICHTIFFIPTC);
        if ((field != (const TIFFField *) NULL) &&
            (TIFFFieldDataType(field) == TIFF_LONG))
          {
            if (TIFFIsByteSwapped(tiff))
              TIFFSwabArrayOfLong((uint32 *) GetStringInfoDatum(iptc_profile),
                (tmsize_t) (length/4));
            (void) TIFFSetField(tiff,TIFFTAG_RICHTIFFIPTC,(uint32)
              GetStringInfoLength(iptc_profile)/4,GetStringInfoDatum(
                iptc_profile));
          }
        else
          (void) TIFFSetField(tiff,TIFFTAG_RICHTIFFIPTC,(uint32)
            GetStringInfoLength(iptc_profile),GetStringInfoDatum(
              iptc_profile));
        iptc_profile=DestroyStringInfo(iptc_profile);
      }
#if defined(TIFFTAG_PHOTOSHOP)
    if (LocaleCompare(name,"8bim") == 0)
      (void) TIFFSetField(tiff,TIFFTAG_PHOTOSHOP,(uint32)
        GetStringInfoLength(profile),GetStringInfoDatum(profile));
#endif
    if (LocaleCompare(name,"tiff:37724") == 0)
      (void) TIFFSetField(tiff,37724,(uint32) GetStringInfoLength(profile),
        GetStringInfoDatum(profile));
    if (LocaleCompare(name,"tiff:34118") == 0)
      (void) TIFFSetField(tiff,34118,(uint32) GetStringInfoLength(profile),
        GetStringInfoDatum(profile));
    name=GetNextImageProfile(image);
  }
}

static void TIFFSetProperties(TIFF *tiff,const MagickBooleanType adjoin,
  Image *image,ExceptionInfo *exception)
{
  const char
    *value;