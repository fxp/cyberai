/* ===== EXTRACT: gif.c ===== */
/* Function: DecodeImage */
/* Lines: 400–512 */

static MagickBooleanType DecodeImage(Image *image,const ssize_t opacity,
  ExceptionInfo *exception)
{
  int
    c;

  LZWInfo
    *lzw_info;

  size_t
    pass;

  ssize_t
    index,
    offset,
    y;

  unsigned char
    data_size;

  /*
    Allocate decoder tables.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  data_size=(unsigned char) ReadBlobByte(image);
  if (data_size > MaximumLZWBits)
    ThrowBinaryException(CorruptImageError,"CorruptImage",image->filename);
  lzw_info=AcquireLZWInfo(image,data_size);
  if (lzw_info == (LZWInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  pass=0;
  offset=0;
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

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
