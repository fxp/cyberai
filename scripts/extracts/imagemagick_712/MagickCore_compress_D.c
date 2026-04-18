// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/compress.c
// Segment 4/11



  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->ascii85 != (Ascii85Info *) NULL);
  image->ascii85->buffer[image->ascii85->offset]=code;
  image->ascii85->offset++;
  if (image->ascii85->offset < 4)
    return;
  p=image->ascii85->buffer;
  for (n=image->ascii85->offset; n >= 4; n-=4)
  {
    Ascii85Tuple(image->ascii85,p);
    for (q=image->ascii85->tuple; *q != '\0'; q++)
    {
      image->ascii85->line_break--;
      if ((image->ascii85->line_break < 0) && (*q != '%'))
        {
          (void) WriteBlobByte(image,'\n');
          image->ascii85->line_break=2*MaxLineExtent;
        }
      (void) WriteBlobByte(image,(unsigned char) *q);
    }
    p+=(ptrdiff_t) 8;
  }
  image->ascii85->offset=n;
  p-=(ptrdiff_t)4;
  for (n=0; n < 4; n++)
    image->ascii85->buffer[n]=(*p++);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H u f f m a n D e c o d e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  HuffmanDecodeImage() uncompresses an image via Huffman-coding.
%
%  The format of the HuffmanDecodeImage method is:
%
%      MagickBooleanType HuffmanDecodeImage(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType HuffmanDecodeImage(Image *image,
  ExceptionInfo *exception)
{
#define HashSize  1021L
#define MBHashA  293L
#define MBHashB  2695L
#define MWHashA  3510L
#define MWHashB  1178L

#define InitializeHashTable(hash,table,a,b) \
{ \
  entry=table; \
  while (entry->code != 0) \
  {  \
    hash[((entry->length+a)*(entry->code+b)) % HashSize]=(HuffmanTable *) entry; \
    entry++; \
  } \
}

#define InputBit(bit)  \
{  \
  if ((mask & 0xff) == 0)  \
    {  \
      byte=ReadBlobByte(image);  \
      if (byte == EOF)  \
        break;  \
      mask=0x80;  \
    }  \
  runlength++;  \
  bit=(size_t) ((byte & mask) != 0 ? 0x01 : 0x00); \
  mask>>=1;  \
  if (bit != 0)  \
    runlength=0;  \
}

  CacheView
    *image_view;

  const HuffmanTable
    *entry;

  HuffmanTable
    **mb_hash,
    **mw_hash;

  int
    byte,
    mask;

  MagickBooleanType
    proceed;

  Quantum
    index;

  size_t
    bit,
    code,
    length,
    null_lines,
    runlength;

  ssize_t
    count,
    i,
    y;

  unsigned char
    *p,
    *scanline;

  unsigned int
    bail,
    color;