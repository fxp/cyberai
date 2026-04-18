// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 5/24



  /*
    Read embedded image.
  */
  length=(size_t) ((MagickOffsetType) GetBlobSize(image)-TellBlob(image));
  pixel_info=AcquireVirtualMemory(length,sizeof(*pixels));
  if (pixel_info == (MemoryInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return((Image *) NULL);
    }
  pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
  stream=ReadBlobStream(image,length,pixels,&count);
  if (count != (ssize_t) length)
    {
      pixel_info=RelinquishVirtualMemory(pixel_info);
      (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
        "ImproperImageHeader","`%s'",image->filename);
      return((Image *) NULL);
    }
  embed_info=AcquireImageInfo();
  (void) FormatLocaleString(embed_info->filename,MagickPathExtent,
    "%s:%s",magick,image_info->filename);
  embed_image=BlobToImage(embed_info,stream,(size_t) count,exception);
  embed_info=DestroyImageInfo(embed_info);
  pixel_info=RelinquishVirtualMemory(pixel_info);
  if (embed_image != (Image *) NULL)
    {
      (void) CopyMagickString(embed_image->filename,image->filename,
        MagickPathExtent);
      (void) CopyMagickString(embed_image->magick_filename,
        image->magick_filename,MagickPathExtent);
      (void) CopyMagickString(embed_image->magick,image->magick,
        MagickPathExtent);
    }
  return(embed_image);
}

static Image *ReadBMPImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  BMPInfo
    bmp_info;

  Image
    *image;

  MagickBooleanType
    ignore_filesize,
    status;

  MagickOffsetType
    offset,
    profile_data,
    profile_size,
    start_position;

  MagickSizeType
    blob_size;

  MemoryInfo
    *pixel_info;

  Quantum
    index,
    *q;

  size_t
    bit,
    bytes_per_line,
    extent,
    length;

  ssize_t
    count,
    i,
    x,
    y;

  unsigned char
    magick[12],
    *p,
    *pixels;

  unsigned int
    blue,
    green,
    offset_bits,
    red;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  image=AcquireImage(image_info,exception);
  image->columns=0;
  image->rows=0;
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Determine if this a BMP file.
  */
  (void) memset(&bmp_info,0,sizeof(bmp_info));
  bmp_info.ba_offset=0;
  start_position=0;
  offset_bits=0;
  count=ReadBlob(image,2,magick);
  if (count != 2)
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  blob_size=GetBlobSize(image);
  do
  {
    PixelInfo
      quantum_bits;

    PixelPacket
      shift;