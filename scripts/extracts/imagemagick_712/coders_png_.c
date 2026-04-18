// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 87/94



  /* Encode image as a JPEG blob */
  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  Creating jpeg_image_info.");
  jpeg_image_info=(ImageInfo *) CloneImageInfo(image_info);
  if (jpeg_image_info == (ImageInfo *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  Creating jpeg_image.");

  jpeg_image=CloneImage(image,0,0,MagickTrue,exception);
  if (jpeg_image == (Image *) NULL)
    {
      jpeg_image_info=DestroyImageInfo(jpeg_image_info);
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    }
  (void) CopyMagickString(jpeg_image->magick,"JPEG",MagickPathExtent);

  (void) AcquireUniqueFilename(jpeg_image->filename);
  (void) FormatLocaleString(jpeg_image_info->filename,MagickPathExtent,"%s",
    jpeg_image->filename);

  status=OpenBlob(jpeg_image_info,jpeg_image,WriteBinaryBlobMode,
    exception);

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  Created jpeg_image, %.20g x %.20g.",(double) jpeg_image->columns,
      (double) jpeg_image->rows);

  if (status == MagickFalse)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");

  if (jng_color_type == 8 || jng_color_type == 12)
    jpeg_image_info->type=GrayscaleType;

  jpeg_image_info->quality=jng_quality;
  jpeg_image->quality=jng_quality;
  (void) CopyMagickString(jpeg_image_info->magick,"JPEG",MagickPathExtent);
  (void) CopyMagickString(jpeg_image->magick,"JPEG",MagickPathExtent);

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  Creating blob.");

  blob=(unsigned char *) ImageToBlob(jpeg_image_info,jpeg_image,&length,
    exception);

  if (blob == (unsigned char *) NULL)
    {
      if (jpeg_image != (Image *)NULL)
        jpeg_image=DestroyImage(jpeg_image);
      if (jpeg_image_info != (ImageInfo *)NULL)
        jpeg_image_info=DestroyImageInfo(jpeg_image_info);
      return(MagickFalse);
    }

  if (logging != MagickFalse)
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  Successfully read jpeg_image into a blob, length=%.20g.",
        (double) length);

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  Write JDAT chunk, length=%.20g.",(double) length);
    }

  /* Write JDAT chunk(s) */
  (void) WriteBlobMSBULong(image,(size_t) length);
  PNGType(chunk,mng_JDAT);
  LogPNGChunk(logging,mng_JDAT,length);
  (void) WriteBlob(image,4,chunk);
  (void) WriteBlob(image,length,blob);
  (void) WriteBlobMSBULong(image,crc32(crc32(0,chunk,4),blob,(uInt) length));

  jpeg_image=DestroyImage(jpeg_image);
  (void) RelinquishUniqueFileResource(jpeg_image_info->filename);
  jpeg_image_info=DestroyImageInfo(jpeg_image_info);
  blob=(unsigned char *) RelinquishMagickMemory(blob);

  /* Write IEND chunk */
  (void) WriteBlobMSBULong(image,0L);
  PNGType(chunk,mng_IEND);
  LogPNGChunk(logging,mng_IEND,0);
  (void) WriteBlob(image,4,chunk);
  (void) WriteBlobMSBULong(image,crc32(0,chunk,4));

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  exit WriteOneJNGImage()");

  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e J N G I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteJNGImage() writes a JPEG Network Graphics (JNG) image file.
%
%  JNG support written by Glenn Randers-Pehrson, glennrp@image...
%
%  The format of the WriteJNGImage method is:
%
%      MagickBooleanType WriteJNGImage(const ImageInfo *image_info,
%        Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image:  The image.
%
%    o exception: return any errors or warnings in this structure.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/
static MagickBooleanType WriteJNGImage(const ImageInfo *image_info,Image *image,
  ExceptionInfo *exception)
{
  MagickBooleanType
    logging = MagickFalse,
    status;

  MngWriteInfo
    *mng_info;