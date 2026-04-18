// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/compress.c
// Segment 3/11



static const HuffmanTable
  TWTable[]=
  {
    { TWId, 0x35, 8, 0 }, { TWId, 0x07, 6, 1 }, { TWId, 0x07, 4, 2 },
    { TWId, 0x08, 4, 3 }, { TWId, 0x0b, 4, 4 }, { TWId, 0x0c, 4, 5 },
    { TWId, 0x0e, 4, 6 }, { TWId, 0x0f, 4, 7 }, { TWId, 0x13, 5, 8 },
    { TWId, 0x14, 5, 9 }, { TWId, 0x07, 5, 10 }, { TWId, 0x08, 5, 11 },
    { TWId, 0x08, 6, 12 }, { TWId, 0x03, 6, 13 }, { TWId, 0x34, 6, 14 },
    { TWId, 0x35, 6, 15 }, { TWId, 0x2a, 6, 16 }, { TWId, 0x2b, 6, 17 },
    { TWId, 0x27, 7, 18 }, { TWId, 0x0c, 7, 19 }, { TWId, 0x08, 7, 20 },
    { TWId, 0x17, 7, 21 }, { TWId, 0x03, 7, 22 }, { TWId, 0x04, 7, 23 },
    { TWId, 0x28, 7, 24 }, { TWId, 0x2b, 7, 25 }, { TWId, 0x13, 7, 26 },
    { TWId, 0x24, 7, 27 }, { TWId, 0x18, 7, 28 }, { TWId, 0x02, 8, 29 },
    { TWId, 0x03, 8, 30 }, { TWId, 0x1a, 8, 31 }, { TWId, 0x1b, 8, 32 },
    { TWId, 0x12, 8, 33 }, { TWId, 0x13, 8, 34 }, { TWId, 0x14, 8, 35 },
    { TWId, 0x15, 8, 36 }, { TWId, 0x16, 8, 37 }, { TWId, 0x17, 8, 38 },
    { TWId, 0x28, 8, 39 }, { TWId, 0x29, 8, 40 }, { TWId, 0x2a, 8, 41 },
    { TWId, 0x2b, 8, 42 }, { TWId, 0x2c, 8, 43 }, { TWId, 0x2d, 8, 44 },
    { TWId, 0x04, 8, 45 }, { TWId, 0x05, 8, 46 }, { TWId, 0x0a, 8, 47 },
    { TWId, 0x0b, 8, 48 }, { TWId, 0x52, 8, 49 }, { TWId, 0x53, 8, 50 },
    { TWId, 0x54, 8, 51 }, { TWId, 0x55, 8, 52 }, { TWId, 0x24, 8, 53 },
    { TWId, 0x25, 8, 54 }, { TWId, 0x58, 8, 55 }, { TWId, 0x59, 8, 56 },
    { TWId, 0x5a, 8, 57 }, { TWId, 0x5b, 8, 58 }, { TWId, 0x4a, 8, 59 },
    { TWId, 0x4b, 8, 60 }, { TWId, 0x32, 8, 61 }, { TWId, 0x33, 8, 62 },
    { TWId, 0x34, 8, 63 }, { TWId, 0x00, 0, 0 }
  };

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A S C I I 8 5 E n c o d e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ASCII85Encode() encodes data in ASCII base-85 format.  ASCII base-85
%  encoding produces five ASCII printing characters from every four bytes of
%  binary data.
%
%  The format of the ASCII85Encode method is:
%
%      void Ascii85Encode(Image *image,const size_t code)
%
%  A description of each parameter follows:
%
%    o code: a binary unsigned char to encode to ASCII 85.
%
%    o file: write the encoded ASCII character to this file.
%
%
*/
static inline void Ascii85Tuple(Ascii85Info *ascii85_info,
  const unsigned char *magick_restrict data)
{
#define MaxLineExtent  36L

  size_t
    code,
    i,
    quantum,
    x;

  code=((((size_t) data[0] << 8) | (size_t) data[1]) << 16) |
    ((size_t) data[2] << 8) | (size_t) data[3];
  if (code == 0L)
    {
      ascii85_info->tuple[0]='z';
      ascii85_info->tuple[1]='\0';
      return;
    }
  quantum=85UL*85UL*85UL*85UL;
  for (i=0; i < 4; i++)
  {
    x=(code/quantum);
    code-=quantum*x;
    ascii85_info->tuple[i]=(char) (x+(int) '!');
    quantum/=85L;
  }
  ascii85_info->tuple[4]=(char) ((code % 85L)+(int) '!');
  ascii85_info->tuple[5]='\0';
}

MagickExport void Ascii85Initialize(Image *image)
{
  /*
    Allocate image structure.
  */
  if (image->ascii85 == (Ascii85Info *) NULL)
    image->ascii85=(Ascii85Info *) AcquireMagickMemory(sizeof(*image->ascii85));
  if (image->ascii85 == (Ascii85Info *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) memset(image->ascii85,0,sizeof(*image->ascii85));
  image->ascii85->line_break=(ssize_t) (MaxLineExtent << 1);
  image->ascii85->offset=0;
}

MagickExport void Ascii85Flush(Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->ascii85 != (Ascii85Info *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->ascii85->offset > 0)
    {
      image->ascii85->buffer[image->ascii85->offset]='\0';
      image->ascii85->buffer[image->ascii85->offset+1]='\0';
      image->ascii85->buffer[image->ascii85->offset+2]='\0';
      Ascii85Tuple(image->ascii85,image->ascii85->buffer);
      (void) WriteBlob(image,(size_t) image->ascii85->offset+1,
        (const unsigned char *) (*image->ascii85->tuple == 'z' ? "!!!!" :
        image->ascii85->tuple));
    }
  (void) WriteBlobByte(image,'~');
  (void) WriteBlobByte(image,'>');
  (void) WriteBlobByte(image,'\n');
}

MagickExport void Ascii85Encode(Image *image,const unsigned char code)
{
  char
    *q;

  unsigned char
    *p;

  ssize_t
    n;