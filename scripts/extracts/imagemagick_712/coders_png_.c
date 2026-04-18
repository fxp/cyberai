// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 78/94

lso
%               from the application programming interfaces.  The options
%               are case-independent and are converted to lowercase before
%               being passed to this encoder.
%
%               png:color-type can be 0, 2, 3, 4, or 6.
%
%               When png:color-type is 0 (Grayscale), png:bit-depth can
%               be 1, 2, 4, 8, or 16.
%
%               When png:color-type is 2 (RGB), png:bit-depth can
%               be 8 or 16.
%
%               When png:color-type is 3 (Indexed), png:bit-depth can
%               be 1, 2, 4, or 8.  This refers to the number of bits
%               used to store the index.  The color samples always have
%               bit-depth 8 in indexed PNG files.
%
%               When png:color-type is 4 (Gray-Matte) or 6 (RGB-Matte),
%               png:bit-depth can be 8 or 16.
%
%               If the image cannot be written without loss with the
%               requested bit-depth and color-type, a PNG file will not
%               be written, a warning will be issued, and the encoder will
%               return MagickFalse.
%
%  Since image encoders should not be responsible for the "heavy lifting",
%  the user should make sure that ImageMagick has already reduced the
%  image depth and number of colors and limit transparency to binary
%  transparency prior to attempting to write the image with depth, color,
%  or transparency limitations.
%
%  Note that another definition, "png:bit-depth-written" exists, but it
%  is not intended for external use.  It is only used internally by the
%  PNG encoder to inform the JNG encoder of the depth of the alpha channel.
%
%  As of version 6.6.6 the following optimizations are always done:
%
%   o  32-bit depth is reduced to 16.
%   o  16-bit depth is reduced to 8 if all pixels contain samples whose
%      high byte and low byte are identical.
%   o  Palette is sorted to remove unused entries and to put a
%      transparent color first, if BUILD_PNG_PALETTE is defined.
%   o  Opaque matte channel is removed when writing an indexed PNG.
%   o  Grayscale images are reduced to 1, 2, or 4 bit depth if
%      this can be done without loss and a larger bit depth N was not
%      requested via the "-define png:bit-depth=N" option.
%   o  If matte channel is present but only one transparent color is
%      present, RGB+tRNS is written instead of RGBA
%   o  Opaque matte channel is removed (or added, if color-type 4 or 6
%      was requested when converting an opaque image).
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/
static MagickBooleanType WritePNGImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  MagickBooleanType
    excluding,
    logging = MagickFalse,
    status;

  MngWriteInfo
    *mng_info;

  const char
    *value;

  int
    source;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->debug != MagickFalse)
    logging=LogMagickEvent(CoderEvent,GetMagickModule(),
      "Enter WritePNGImage()");
  mng_info=(MngWriteInfo *) AcquireMagickMemory(sizeof(*mng_info));
  if (mng_info == (MngWriteInfo *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memset(mng_info,0,sizeof(*mng_info));
  mng_info->image=image;
  mng_info->equal_backgrounds=MagickTrue;

  /* See if user has requested a specific PNG subformat */

  mng_info->write_png8=LocaleCompare(image_info->magick,"PNG8") == 0 ?
    MagickTrue : MagickFalse;
  mng_info->write_png24=LocaleCompare(image_info->magick,"PNG24") == 0 ?
    MagickTrue : MagickFalse;
  mng_info->write_png32=LocaleCompare(image_info->magick,"PNG32") == 0 ?
    MagickTrue : MagickFalse;
  mng_info->write_png48=LocaleCompare(image_info->magick,"PNG48") == 0 ?
    MagickTrue : MagickFalse;
  mng_info->write_png64=LocaleCompare(image_info->magick,"PNG64") == 0 ?
    MagickTrue : MagickFalse;

  value=GetImageOption(image_info,"png:format");

  if (value != (char *) NULL || LocaleCompare(image_info->magick,"PNG00") == 0)
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
         "  Format=%s",value);

      mng_info->write_png8 = MagickFalse;
      mng_info->write_png24 = MagickFalse;
      mng_info->write_png32 = MagickFalse;
      mng_info->write_png48 = MagickFalse;
      mng_info->write_png64 = MagickFalse;

      if (LocaleCompare(value,"png8") == 0)
        mng_info->write_png8 = MagickTrue;

      else if (LocaleCompare(value,"png24") == 0)
        mng_info->write_png24 = MagickTrue;

      else if (LocaleCompare(value,"png32") == 0)
        mng_info->write_png32 = MagickTrue;