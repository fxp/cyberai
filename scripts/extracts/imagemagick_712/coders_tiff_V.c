// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 22/34



  /*
    Write image as CCITT Group4 TIFF image to a temporary file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(status);
  huffman_image=CloneImage(image,0,0,MagickTrue,exception);
  if (huffman_image == (Image *) NULL)
    {
      (void) CloseBlob(image);
      return(MagickFalse);
    }
  huffman_image->endian=MSBEndian;
  file=(FILE *) NULL;
  unique_file=AcquireUniqueFileResource(filename);
  if (unique_file != -1)
    file=fdopen(unique_file,"wb");
  if ((unique_file == -1) || (file == (FILE *) NULL))
    {
      ThrowFileException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        filename);
      return(MagickFalse);
    }
  (void) FormatLocaleString(huffman_image->filename,MagickPathExtent,"tiff:%s",
    filename);
  if (IsImageMonochrome(huffman_image) == MagickFalse)
    (void) SetImageType(huffman_image,BilevelType,exception);
  write_info=CloneImageInfo((ImageInfo *) NULL);
  SetImageInfoFile(write_info,file);
  if (IsImageMonochrome(image) == MagickFalse)
    (void) SetImageType(image,BilevelType,exception);
  (void) SetImageDepth(image,1,exception);
  write_info->compression=Group4Compression;
  write_info->type=BilevelType;
  status=WriteTIFFImage(write_info,huffman_image,exception);
  (void) fflush(file);
  write_info=DestroyImageInfo(write_info);
  if (status == MagickFalse)
    {
      huffman_image=DestroyImage(huffman_image);
      (void) fclose(file);
      (void) RelinquishUniqueFileResource(filename);
      return(MagickFalse);
    }
  tiff=TIFFOpen(filename,"rb");
  if (tiff == (TIFF *) NULL)
    {
      huffman_image=DestroyImage(huffman_image);
      (void) fclose(file);
      (void) RelinquishUniqueFileResource(filename);
      ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
        image_info->filename);
      return(MagickFalse);
    }
  /*
    Allocate raw strip buffer.
  */
  if (TIFFGetField(tiff,TIFFTAG_STRIPBYTECOUNTS,&byte_count) != 1)
    {
      TIFFClose(tiff);
      huffman_image=DestroyImage(huffman_image);
      (void) fclose(file);
      (void) RelinquishUniqueFileResource(filename);
      return(MagickFalse);
    }
  strip_size=byte_count[0];
  for (i=1; i < (ssize_t) TIFFNumberOfStrips(tiff); i++)
    if (byte_count[i] > strip_size)
      strip_size=byte_count[i];
  buffer=(unsigned char *) AcquireQuantumMemory((size_t) strip_size,
    sizeof(*buffer));
  if (buffer == (unsigned char *) NULL)
    {
      TIFFClose(tiff);
      huffman_image=DestroyImage(huffman_image);
      (void) fclose(file);
      (void) RelinquishUniqueFileResource(filename);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image_info->filename);
    }
  /*
    Compress runlength encoded to 2D Huffman pixels.
  */
  for (i=0; i < (ssize_t) TIFFNumberOfStrips(tiff); i++)
  {
    count=(ssize_t) TIFFReadRawStrip(tiff,(uint32) i,buffer,(tmsize_t)
      strip_size);
    if (WriteBlob(image,(size_t) count,buffer) != count)
      status=MagickFalse;
  }
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  TIFFClose(tiff);
  huffman_image=DestroyImage(huffman_image);
  (void) fclose(file);
  (void) RelinquishUniqueFileResource(filename);
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  return(status);
}
#endif

#if defined(MAGICKCORE_TIFF_DELEGATE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P T I F I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WritePTIFImage() writes an image in the pyramid-encoded Tagged image file
%  format.
%
%  The format of the WritePTIFImage method is:
%
%      MagickBooleanType WritePTIFImage(const ImageInfo *image_info,
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
*/
static MagickBooleanType WritePTIFImage(const ImageInf