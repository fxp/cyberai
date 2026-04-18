// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 8/34



  /*
    Non-CMYK JPEG TIFFs can be decoded with the strip method when 8-bit,
    fall back to the generic method for other bit depths.
  */
  if (photometric != PHOTOMETRIC_SEPARATED)
    {
      if (bits_per_sample != 8)
        return(ReadGenericMethod);
      return(ReadStripMethod);
    }
  /* Only 8-bit and 4-sample are supported for the APP14 marker probe */
  if ((bits_per_sample != 8) || (samples_per_pixel != 4))
    return(ReadStripMethod);
  if (!TIFFGetField(tiff,TIFFTAG_STRIPOFFSETS,&value) || (value == NULL))
    return(ReadStripMethod);
  position=TellBlob(image);
  offset=(MagickOffsetType) (value[0]);
  if (SeekBlob(image,offset,SEEK_SET) != offset)
    return(ReadStripMethod);
  method=ReadStripMethod;
  if (ReadBlob(image,BUFFER_SIZE,buffer) == BUFFER_SIZE)
    {
      for (i=0; i < BUFFER_SIZE; i++)
      {
        while (i < BUFFER_SIZE)
        {
          if (buffer[i++] == 255)
           break;
        }
        while (i < BUFFER_SIZE)
        {
          if (buffer[++i] != 255)
           break;
        }
        if (buffer[i++] == 216) /* JPEG_MARKER_SOI */
          continue;
        length=(unsigned short) (((unsigned int) (buffer[i] << 8) |
          (unsigned int) buffer[i+1]) & 0xffff);
        if (i+(size_t) length >= BUFFER_SIZE)
          break;
        if (buffer[i-1] == 238) /* JPEG_MARKER_APP0+14 */
          {
            if (length != 14)
              break;
            /* 0 == CMYK, 1 == YCbCr, 2 = YCCK */
            if (buffer[i+13] == 2)
              method=ReadYCCKMethod;
            break;
          }
        i+=(size_t) length;
      }
    }
  (void) SeekBlob(image,position,SEEK_SET);
  return(method);
}

static ssize_t TIFFReadCustomStream(unsigned char *data,const size_t count,
  void *user_data)
{
  PhotoshopProfile
    *profile;

  size_t
    total;

  MagickOffsetType
    remaining;

  if (count == 0)
    return(0);
  profile=(PhotoshopProfile *) user_data;
  remaining=(MagickOffsetType) profile->length-profile->offset;
  if (remaining <= 0)
    return(-1);
  total=MagickMin(count, (size_t) remaining);
  (void) memcpy(data,profile->data->datum+profile->offset,total);
  profile->offset+=(MagickOffsetType) total;
  return((ssize_t) total);
}

static CustomStreamInfo *TIFFAcquireCustomStreamForReading(
  PhotoshopProfile *profile,ExceptionInfo *exception)
{
  CustomStreamInfo
    *custom_stream;

  custom_stream=AcquireCustomStreamInfo(exception);
  if (custom_stream == (CustomStreamInfo *) NULL)
    return(custom_stream);
  SetCustomStreamData(custom_stream,(void *) profile);
  SetCustomStreamReader(custom_stream,TIFFReadCustomStream);
  SetCustomStreamSeeker(custom_stream,TIFFSeekCustomStream);
  SetCustomStreamTeller(custom_stream,TIFFTellCustomStream);
  return(custom_stream);
}

static void TIFFReadPhotoshopLayers(const ImageInfo *image_info,Image *image,
  ExceptionInfo *exception)
{
  const char
    *option;

  const StringInfo
    *profile;

  ExceptionInfo
    *sans_exception;

  CustomStreamInfo
    *custom_stream;

  Image
    *layers;

  ImageInfo
    *clone_info;

  PhotoshopProfile
    photoshop_profile;

  PSDInfo
    info;

  ssize_t
    i;