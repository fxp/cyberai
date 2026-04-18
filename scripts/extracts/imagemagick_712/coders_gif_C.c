// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 3/18



    lzw_info->code_info.buffer[0]=lzw_info->code_info.buffer[
      lzw_info->code_info.count-2];
    lzw_info->code_info.buffer[1]=lzw_info->code_info.buffer[
      lzw_info->code_info.count-1];
    lzw_info->code_info.bit-=8*(lzw_info->code_info.count-2);
    lzw_info->code_info.count=2;
    count=ReadBlobBlock(lzw_info->image,&lzw_info->code_info.buffer[
      lzw_info->code_info.count]);
    if (count > 0)
      lzw_info->code_info.count+=(size_t) count;
    else
      lzw_info->code_info.eof=MagickTrue;
  }
  if ((lzw_info->code_info.bit+bits) > (8*lzw_info->code_info.count))
    return(-1);
  code=0;
  one=1;
  for (i=0; i < (ssize_t) bits; i++)
  {
    code|=((lzw_info->code_info.buffer[lzw_info->code_info.bit/8] &
      (one << (lzw_info->code_info.bit % 8))) != 0) << i;
    lzw_info->code_info.bit++;
  }
  return(code);
}

static inline int PopLZWStack(LZWStack *stack_info)
{
  if (stack_info->index <= stack_info->codes)
    return(-1);
  stack_info->index--;
  return((int) *stack_info->index);
}

static inline void PushLZWStack(LZWStack *stack_info,const size_t value)
{
  if (stack_info->index >= stack_info->top)
    return;
  *stack_info->index=value;
  stack_info->index++;
}

static int ReadBlobLZWByte(LZWInfo *lzw_info)
{
  int
    code;

  size_t
    one,
    value;

  ssize_t
    count;

  if (lzw_info->stack->index != lzw_info->stack->codes)
    return(PopLZWStack(lzw_info->stack));
  if (lzw_info->genesis != MagickFalse)
    {
      lzw_info->genesis=MagickFalse;
      do
      {
        lzw_info->first_code=(size_t) GetNextLZWCode(lzw_info,lzw_info->bits);
        lzw_info->last_code=lzw_info->first_code;
      } while (lzw_info->first_code == lzw_info->clear_code);
      return((int) lzw_info->first_code);
    }
  code=GetNextLZWCode(lzw_info,lzw_info->bits);
  if (code < 0)
    return(code);
  if ((size_t) code == lzw_info->clear_code)
    {
      ResetLZWInfo(lzw_info);
      return(ReadBlobLZWByte(lzw_info));
    }
  if ((size_t) code == lzw_info->end_code)
    return(-1);
  if ((size_t) code < lzw_info->slot)
    value=(size_t) code;
  else
    {
      PushLZWStack(lzw_info->stack,lzw_info->first_code);
      value=lzw_info->last_code;
    }
  count=0;
  while (value > lzw_info->maximum_data_value)
  {
    if ((size_t) count > MaximumLZWCode)
      return(-1);
    count++;
    if ((size_t) value > MaximumLZWCode)
      return(-1);
    PushLZWStack(lzw_info->stack,lzw_info->table[1][value]);
    value=lzw_info->table[0][value];
  }
  lzw_info->first_code=lzw_info->table[1][value];
  PushLZWStack(lzw_info->stack,lzw_info->first_code);
  one=1;
  if (lzw_info->slot < MaximumLZWCode)
    {
      lzw_info->table[0][lzw_info->slot]=lzw_info->last_code;
      lzw_info->table[1][lzw_info->slot]=lzw_info->first_code;
      lzw_info->slot++;
      if ((lzw_info->slot >= lzw_info->maximum_code) &&
          (lzw_info->bits < MaximumLZWBits))
        {
          lzw_info->bits++;
          lzw_info->maximum_code=one << lzw_info->bits;
        }
    }
  lzw_info->last_code=(size_t) code;
  return(PopLZWStack(lzw_info->stack));
}

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