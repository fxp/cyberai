// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 4/18



    q=QueueAuthenticPixels(image,0,offset,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; )
    {
      c=ReadBlobLZWByte(lzw_info);
      if (c < 0)
        break;
      index=ConstrainColormapIndex(image,(ssize_t) c,exception);
      SetPixelIndex(image,(Quantum) index,q);
      SetPixelViaPixelInfo(image,image->colormap+index,q);
      SetPixelAlpha(image,index == opacity ? TransparentAlpha : OpaqueAlpha,q);
      x++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
    if (x < (ssize_t) image->columns)
      break;
    if (image->interlace == NoInterlace)
      offset++;
    else
      {
        switch (pass)
        {
          case 0:
          default:
          {
            offset+=8;
            break;
          }
          case 1:
          {
            offset+=8;
            break;
          }
          case 2:
          {
            offset+=4;
            break;
          }
          case 3:
          {
            offset+=2;
            break;
          }
        }
      if ((pass == 0) && (offset >= (ssize_t) image->rows))
        {
          pass++;
          offset=4;
        }
      if ((pass == 1) && (offset >= (ssize_t) image->rows))
        {
          pass++;
          offset=2;
        }
      if ((pass == 2) && (offset >= (ssize_t) image->rows))
        {
          pass++;
          offset=1;
        }
    }
  }
  lzw_info=RelinquishLZWInfo(lzw_info);
  if (y < (ssize_t) image->rows)
    ThrowBinaryException(CorruptImageError,"CorruptImage",image->filename);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E n c o d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EncodeImage compresses an image via GIF-coding.
%
%  The format of the EncodeImage method is:
%
%      MagickBooleanType EncodeImage(const ImageInfo *image_info,Image *image,
%        const size_t data_size)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image: the address of a structure of type Image.
%
%    o data_size:  The number of bits in the compressed packet.
%
*/
static MagickBooleanType EncodeImage(const ImageInfo *image_info,Image *image,
  const size_t data_size,ExceptionInfo *exception)
{
#define MaxCode(number_bits)  ((one << (number_bits))-1)
#define MaxHashTable  5003
#define MaxGIFBits  12UL
#define MaxGIFTable  (1UL << MaxGIFBits)
#define GIFOutputCode(code) \
{ \
  /*  \
    Emit a code. \
  */ \
  if (bits > 0) \
    datum|=(size_t) (code) << bits; \
  else \
    datum=(size_t) (code); \
  bits+=number_bits; \
  while (bits >= 8) \
  { \
    /*  \
      Add a character to current packet.  Maximum packet size is 255.
    */ \
    packet[length++]=(unsigned char) (datum & 0xff); \
    if (length == 255) \
      { \
        (void) WriteBlobByte(image,(unsigned char) length); \
        (void) WriteBlob(image,length,packet); \
        length=0; \
      } \
    datum>>=8; \
    bits-=8; \
  } \
  if (free_code > max_code)  \
    { \
      number_bits++; \
      if (number_bits == MaxGIFBits) \
        max_code=MaxGIFTable; \
      else \
        max_code=MaxCode(number_bits); \
    } \
}

  Quantum
    index;

  short
    *hash_code,
    *hash_prefix,
    waiting_code;

  size_t
    bits,
    clear_code,
    datum,
    end_of_information_code,
    free_code,
    length,
    max_code,
    next_pixel,
    number_bits,
    one,
    pass;

  ssize_t
    displacement,
    offset,
    k,
    y;

  unsigned char
    *packet,
    *hash_suffix;