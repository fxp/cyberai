// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/compress.c
// Segment 10/11



  /*
    Compress pixels with Packbits encoding.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(pixels != (unsigned char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  packbits=(unsigned char *) AcquireQuantumMemory(128UL,sizeof(*packbits));
  if (packbits == (unsigned char *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  for (i=(ssize_t) length; i != 0; )
  {
    switch (i)
    {
      case 1:
      {
        i--;
        (void) WriteBlobByte(image,(unsigned char) 0);
        (void) WriteBlobByte(image,*pixels);
        break;
      }
      case 2:
      {
        i-=2;
        (void) WriteBlobByte(image,(unsigned char) 1);
        (void) WriteBlobByte(image,*pixels);
        (void) WriteBlobByte(image,pixels[1]);
        break;
      }
      case 3:
      {
        i-=3;
        if ((*pixels == *(pixels+1)) && (*(pixels+1) == *(pixels+2)))
          {
            (void) WriteBlobByte(image,(unsigned char) ((256-3)+1));
            (void) WriteBlobByte(image,*pixels);
            break;
          }
        (void) WriteBlobByte(image,(unsigned char) 2);
        (void) WriteBlobByte(image,*pixels);
        (void) WriteBlobByte(image,pixels[1]);
        (void) WriteBlobByte(image,pixels[2]);
        break;
      }
      default:
      {
        if ((*pixels == *(pixels+1)) && (*(pixels+1) == *(pixels+2)))
          {
            /*
              Packed run.
            */
            count=3;
            while (((ssize_t) count < i) && (*pixels == *(pixels+count)))
            {
              count++;
              if (count >= 127)
                break;
            }
            i-=count;
            (void) WriteBlobByte(image,(unsigned char) ((256-count)+1));
            (void) WriteBlobByte(image,*pixels);
            pixels+=count;
            break;
          }
        /*
          Literal run.
        */
        count=0;
        while ((*(pixels+count) != *(pixels+count+1)) ||
               (*(pixels+count+1) != *(pixels+count+2)))
        {
          packbits[count+1]=pixels[count];
          count++;
          if (((ssize_t) count >= (i-3)) || (count >= 127))
            break;
        }
        i-=count;
        *packbits=(unsigned char) (count-1);
        for (j=0; j <= (ssize_t) count; j++)
          (void) WriteBlobByte(image,packbits[j]);
        pixels+=count;
        break;
      }
    }
  }
  (void) WriteBlobByte(image,(unsigned char) 128);  /* EOD marker */
  packbits=(unsigned char *) RelinquishMagickMemory(packbits);
  return(MagickTrue);
}

#if defined(MAGICKCORE_ZLIB_DELEGATE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   Z L I B E n c o d e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ZLIBEncodeImage compresses an image via ZLIB-coding specific to
%  Postscript Level II or Portable Document Format.
%
%  The format of the ZLIBEncodeImage method is:
%
%      MagickBooleanType ZLIBEncodeImage(Image *image,const size_t length,
%        unsigned char *magick_restrict pixels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o file: the address of a structure of type FILE.  ZLIB encoded pixels
%      are written to this file.
%
%    o length:  A value that specifies the number of pixels to compress.
%
%    o pixels: the address of an unsigned array of characters containing the
%      pixels to compress.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static voidpf AcquireZIPMemory(voidpf context,unsigned int items,
  unsigned int size)
{
  (void) context;
  return((voidpf) AcquireQuantumMemory(items,size));
}

static void RelinquishZIPMemory(voidpf context,voidpf memory)
{
  (void) context;
  memory=RelinquishMagickMemory(memory);
}

MagickExport MagickBooleanType ZLIBEncodeImage(Image *image,const size_t length,
  unsigned char *magick_restrict pixels,ExceptionInfo *exception)
{
  int
    status;

  ssize_t
    i;

  size_t
    compress_packets;

  unsigned char
    *compress_pixels;

  z_stream
    stream;