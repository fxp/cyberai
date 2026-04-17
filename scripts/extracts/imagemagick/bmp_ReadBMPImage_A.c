/* ===== EXTRACT: bmp.c ===== */
/* Function: ReadBMPImage (part A) */
/* Lines: 589–749 */

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

    /*
      Verify BMP identifier.
    */
    start_position=TellBlob(image)-2;
    bmp_info.ba_offset=0;
    while (LocaleNCompare((char *) magick,"BA",2) == 0)
    {
      bmp_info.file_size=ReadBlobLSBLong(image);
      bmp_info.ba_offset=ReadBlobLSBLong(image);
      bmp_info.offset_bits=ReadBlobLSBLong(image);
      count=ReadBlob(image,2,magick);
      if (count != 2)
        break;
    }
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  Magick: %c%c",
        magick[0],magick[1]);
    if ((count != 2) || ((LocaleNCompare((char *) magick,"BM",2) != 0) &&
        (LocaleNCompare((char *) magick,"CI",2) != 0)))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    bmp_info.file_size=ReadBlobLSBLong(image);
    (void) ReadBlobLSBLong(image);
    bmp_info.offset_bits=ReadBlobLSBLong(image);
    bmp_info.size=ReadBlobLSBLong(image);
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  BMP header size: %u",bmp_info.size);
    if (LocaleNCompare((char *) magick,"CI",2) == 0)
      {
        if ((bmp_info.size != 12) && (bmp_info.size != 40) &&
            (bmp_info.size != 64))
          ThrowReaderException(CorruptImageError,"ImproperImageHeader");
      }
    if (bmp_info.size > 124)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    /*
      Get option to bypass file size check
    */
    ignore_filesize=IsStringTrue(GetImageOption(image_info,
      "bmp:ignore-filesize"));
    if ((ignore_filesize == MagickFalse) && (bmp_info.file_size != 0) &&
        ((MagickSizeType) bmp_info.file_size > GetBlobSize(image)))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    if (bmp_info.offset_bits < bmp_info.size)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    bmp_info.offset_bits=MagickMax(14+bmp_info.size,bmp_info.offset_bits);
    profile_data=0;
    profile_size=0;
    if (bmp_info.size == 12)
      {
        /*
          OS/2 BMP image file.
        */
        (void) CopyMagickString(image->magick,"BMP2",MagickPathExtent);
        bmp_info.width=(ssize_t) ((short) ReadBlobLSBShort(image));
        bmp_info.height=(ssize_t) ((short) ReadBlobLSBShort(image));
        bmp_info.planes=ReadBlobLSBShort(image);
        bmp_info.bits_per_pixel=ReadBlobLSBShort(image);
        bmp_info.x_pixels=0;
        bmp_info.y_pixels=0;
        bmp_info.number_colors=0;
        bmp_info.compression=BI_RGB;
        bmp_info.image_size=0;
        bmp_info.alpha_mask=0;
        if (image->debug != MagickFalse)
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Format: OS/2 Bitmap");
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Geometry: %.20gx%.20g",(double) bmp_info.width,(double)
              bmp_info.height);
          }
      }
