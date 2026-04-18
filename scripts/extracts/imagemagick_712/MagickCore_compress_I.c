// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/compress.c
// Segment 9/11



  /*
    Allocate string table.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(pixels != (unsigned char *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  table=(TableType *) AcquireQuantumMemory(1UL << 12,sizeof(*table));
  if (table == (TableType *) NULL)
    ThrowBinaryException(ResourceLimitWarning,"MemoryAllocationFailed",
      image->filename);
  /*
    Initialize variables.
  */
  accumulator=0;
  code_width=9;
  number_bits=0;
  last_code=0;
  OutputCode(LZWClr);
  for (index=0; index < 256; index++)
  {
    table[index].prefix=(-1);
    table[index].suffix=(ssize_t) index;
    table[index].next=(-1);
  }
  next_index=LZWEod+1;
  code_width=9;
  last_code=(size_t) pixels[0];
  for (i=1; i < (ssize_t) length; i++)
  {
    /*
      Find string.
    */
    index=(ssize_t) last_code;
    while (index != -1)
      if ((table[index].prefix != (ssize_t) last_code) ||
          (table[index].suffix != (ssize_t) pixels[i]))
        index=table[index].next;
      else
        {
          last_code=(size_t) index;
          break;
        }
    if (last_code != (size_t) index)
      {
        /*
          Add string.
        */
        OutputCode(last_code);
        table[next_index].prefix=(ssize_t) last_code;
        table[next_index].suffix=(ssize_t) pixels[i];
        table[next_index].next=table[last_code].next;
        table[last_code].next=(ssize_t) next_index;
        next_index++;
        /*
          Did we just move up to next bit width?
        */
        if ((next_index >> code_width) != 0)
          {
            code_width++;
            if (code_width > 12)
              {
                /*
                  Did we overflow the max bit width?
                */
                code_width--;
                OutputCode(LZWClr);
                for (index=0; index < 256; index++)
                {
                  table[index].prefix=(-1);
                  table[index].suffix=index;
                  table[index].next=(-1);
                }
                next_index=LZWEod+1;
                code_width=9;
              }
            }
          last_code=(size_t) pixels[i];
      }
  }
  /*
    Flush tables.
  */
  OutputCode(last_code);
  OutputCode(LZWEod);
  if (number_bits != 0)
    (void) WriteBlobByte(image,(unsigned char) (accumulator >> 24));
  table=(TableType *) RelinquishMagickMemory(table);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a c k b i t s E n c o d e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PackbitsEncodeImage() compresses an image via Macintosh Packbits encoding
%  specific to Postscript Level II or Portable Document Format.  To ensure
%  portability, the binary Packbits bytes are encoded as ASCII Base-85.
%
%  The format of the PackbitsEncodeImage method is:
%
%      MagickBooleanType PackbitsEncodeImage(Image *image,const size_t length,
%        unsigned char *magick_restrict pixels)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o length:  A value that specifies the number of pixels to compress.
%
%    o pixels: the address of an unsigned array of characters containing the
%      pixels to compress.
%
*/
MagickExport MagickBooleanType PackbitsEncodeImage(Image *image,
  const size_t length,unsigned char *magick_restrict pixels,
  ExceptionInfo *exception)
{
  int
    count;

  ssize_t
    i,
    j;

  unsigned char
    *packbits;