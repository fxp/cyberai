// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/compress.c
// Segment 11/11



  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  compress_packets=(size_t) (1.001*length+12);
  compress_pixels=(unsigned char *) AcquireQuantumMemory(compress_packets,
    sizeof(*compress_pixels));
  if (compress_pixels == (unsigned char *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  (void) memset(&stream,0,sizeof(stream));
  stream.next_in=pixels;
  stream.avail_in=(unsigned int) length;
  stream.next_out=compress_pixels;
  stream.avail_out=(unsigned int) compress_packets;
  stream.zalloc=AcquireZIPMemory;
  stream.zfree=RelinquishZIPMemory;
  stream.opaque=(voidpf) NULL;
  status=deflateInit(&stream,(int) (image->quality ==
    UndefinedCompressionQuality ? 7 : MagickMin(image->quality/10,9)));
  if (status == Z_OK)
    {
      status=deflate(&stream,Z_FINISH);
      if (status == Z_STREAM_END)
        status=deflateEnd(&stream);
      else
        (void) deflateEnd(&stream);
      compress_packets=(size_t) stream.total_out;
    }
  if (status != Z_OK)
    ThrowBinaryException(CoderError,"UnableToZipCompressImage",image->filename)
  for (i=0; i < (ssize_t) compress_packets; i++)
    (void) WriteBlobByte(image,compress_pixels[i]);
  compress_pixels=(unsigned char *) RelinquishMagickMemory(compress_pixels);
  return(MagickTrue);
}
#else
MagickExport MagickBooleanType ZLIBEncodeImage(Image *image,
  const size_t magick_unused(length),unsigned char *magick_unused(pixels),
  ExceptionInfo *exception)
{
  magick_unreferenced(length);
  magick_unreferenced(pixels);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  (void) ThrowMagickException(exception,GetMagickModule(),MissingDelegateError,
    "DelegateLibrarySupportNotBuiltIn","'%s' (ZIP)",image->filename);
  return(MagickFalse);
}
#endif
