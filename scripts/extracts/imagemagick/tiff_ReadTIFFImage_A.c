/* ===== EXTRACT: tiff.c ===== */
/* Function: ReadTIFFImage (part A) */
/* Lines: 1167–1317 */

static Image *ReadTIFFImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
#define ThrowTIFFException(severity,message) \
{ \
  if (pixel_info != (MemoryInfo *) NULL) \
    pixel_info=RelinquishVirtualMemory(pixel_info); \
  if (quantum_info != (QuantumInfo *) NULL) \
    quantum_info=DestroyQuantumInfo(quantum_info); \
  TIFFClose(tiff); \
  ThrowReaderException(severity,message); \
}

  float
    *chromaticity = (float *) NULL,
    x_position,
    y_position,
    x_resolution,
    y_resolution;

  Image
    *image;

  int
    tiff_status = 0;

  MagickBooleanType
    more_frames;

  MagickSizeType
    number_pixels;

  MagickStatusType
    status;

  MemoryInfo
    *pixel_info = (MemoryInfo *) NULL;

  QuantumInfo
    *quantum_info;

  QuantumType
    quantum_type;

  ssize_t
    i,
    scanline_size,
    y;

  TIFF
    *tiff;

  TIFFMethodType
    method;

  uint16
    compress_tag = 0,
    bits_per_sample = 0,
    endian = 0,
    extra_samples = 0,
    interlace = 0,
    max_sample_value = 0,
    min_sample_value = 0,
    orientation = 0,
    pages = 0,
    photometric = 0,
    *sample_info = NULL,
    sample_format = 0,
    samples_per_pixel = 0,
    units = 0,
    value = 0;

  uint32
    height,
    rows_per_strip,
    width;

  unsigned char
    *pixels;

  void
    *sans[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

  /*
    Open image.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  (void) SetMagickThreadValue(tiff_exception,exception);
  tiff=TIFFClientOpen(image->filename,"rb",(thandle_t) image,TIFFReadBlob,
    TIFFWriteBlob,TIFFSeekBlob,TIFFCloseBlob,TIFFGetBlobSize,TIFFMapBlob,
    TIFFUnmapBlob);
  if (tiff == (TIFF *) NULL)
    {
      if (exception->severity == UndefinedException)
        ThrowReaderException(CorruptImageError,"UnableToReadImageData");
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  if (exception->severity > ErrorException)
    {
      TIFFClose(tiff);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  if (image_info->number_scenes != 0)
    {
      /*
        Generate blank images for subimage specification (e.g. image.tif[4].
        We need to check the number of directories because it is possible that
        the subimage(s) are stored in the photoshop profile.
      */
      if (image_info->scene < (size_t) TIFFNumberOfDirectories(tiff))
        {
          for (i=0; i < (ssize_t) image_info->scene; i++)
          {
            status=TIFFReadDirectory(tiff) != 0 ? MagickTrue : MagickFalse;
            if (status == MagickFalse)
              {
                TIFFClose(tiff);
                image=DestroyImageList(image);
                return((Image *) NULL);
              }
            AcquireNextImage(image_info,image,exception);
            if (GetNextImageInList(image) == (Image *) NULL)
              {
                TIFFClose(tiff);
                image=DestroyImageList(image);
                return((Image *) NULL);
              }
            image=SyncNextImageInList(image);
          }
      }
  }
  more_frames=MagickTrue;
  do
  {
    /* TIFFPrintDirectory(tiff,stdout,MagickFalse); */
    photometric=PHOTOMETRIC_RGB;
