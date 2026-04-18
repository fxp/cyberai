// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 2/18



static ssize_t
  ReadBlobBlock(Image *,unsigned char *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e c o d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DecodeImage uncompresses an image via GIF-coding.
%
%  The format of the DecodeImage method is:
%
%      MagickBooleanType DecodeImage(Image *image,const ssize_t opacity)
%
%  A description of each parameter follows:
%
%    o image: the address of a structure of type Image.
%
%    o opacity:  The colormap index associated with the transparent color.
%
*/

static LZWInfo *RelinquishLZWInfo(LZWInfo *lzw_info)
{
  if (lzw_info->table[0] != (size_t *) NULL)
    lzw_info->table[0]=(size_t *) RelinquishMagickMemory(
      lzw_info->table[0]);
  if (lzw_info->table[1] != (size_t *) NULL)
    lzw_info->table[1]=(size_t *) RelinquishMagickMemory(
      lzw_info->table[1]);
  if (lzw_info->stack != (LZWStack *) NULL)
    {
      if (lzw_info->stack->codes != (size_t *) NULL)
        lzw_info->stack->codes=(size_t *) RelinquishMagickMemory(
          lzw_info->stack->codes);
      lzw_info->stack=(LZWStack *) RelinquishMagickMemory(lzw_info->stack);
    }
  lzw_info=(LZWInfo *) RelinquishMagickMemory(lzw_info);
  return((LZWInfo *) NULL);
}

static inline void ResetLZWInfo(LZWInfo *lzw_info)
{
  size_t
    one;

  lzw_info->bits=lzw_info->data_size+1;
  one=1;
  lzw_info->maximum_code=one << lzw_info->bits;
  lzw_info->slot=lzw_info->maximum_data_value+3;
  lzw_info->genesis=MagickTrue;
}

static LZWInfo *AcquireLZWInfo(Image *image,const size_t data_size)
{
  LZWInfo
    *lzw_info;

  ssize_t
    i;

  size_t
    one;

  lzw_info=(LZWInfo *) AcquireMagickMemory(sizeof(*lzw_info));
  if (lzw_info == (LZWInfo *) NULL)
    return((LZWInfo *) NULL);
  (void) memset(lzw_info,0,sizeof(*lzw_info));
  lzw_info->image=image;
  lzw_info->data_size=data_size;
  one=1;
  lzw_info->maximum_data_value=(one << data_size)-1;
  lzw_info->clear_code=lzw_info->maximum_data_value+1;
  lzw_info->end_code=lzw_info->maximum_data_value+2;
  lzw_info->table[0]=(size_t *) AcquireQuantumMemory(MaximumLZWCode,
    sizeof(**lzw_info->table));
  lzw_info->table[1]=(size_t *) AcquireQuantumMemory(MaximumLZWCode,
    sizeof(**lzw_info->table));
  if ((lzw_info->table[0] == (size_t *) NULL) ||
      (lzw_info->table[1] == (size_t *) NULL))
    {
      lzw_info=RelinquishLZWInfo(lzw_info);
      return((LZWInfo *) NULL);
    }
  (void) memset(lzw_info->table[0],0,MaximumLZWCode*sizeof(**lzw_info->table));
  (void) memset(lzw_info->table[1],0,MaximumLZWCode*sizeof(**lzw_info->table));
  for (i=0; i <= (ssize_t) lzw_info->maximum_data_value; i++)
  {
    lzw_info->table[0][i]=0;
    lzw_info->table[1][i]=(size_t) i;
  }
  ResetLZWInfo(lzw_info);
  lzw_info->code_info.buffer[0]='\0';
  lzw_info->code_info.buffer[1]='\0';
  lzw_info->code_info.count=2;
  lzw_info->code_info.bit=8*lzw_info->code_info.count;
  lzw_info->code_info.eof=MagickFalse;
  lzw_info->genesis=MagickTrue;
  lzw_info->stack=(LZWStack *) AcquireMagickMemory(sizeof(*lzw_info->stack));
  if (lzw_info->stack == (LZWStack *) NULL)
    {
      lzw_info=RelinquishLZWInfo(lzw_info);
      return((LZWInfo *) NULL);
    }
  lzw_info->stack->codes=(size_t *) AcquireQuantumMemory(2UL*
    MaximumLZWCode,sizeof(*lzw_info->stack->codes));
  if (lzw_info->stack->codes == (size_t *) NULL)
    {
      lzw_info=RelinquishLZWInfo(lzw_info);
      return((LZWInfo *) NULL);
    }
  lzw_info->stack->index=lzw_info->stack->codes;
  lzw_info->stack->top=lzw_info->stack->codes+2*MaximumLZWCode;
  return(lzw_info);
}

static inline int GetNextLZWCode(LZWInfo *lzw_info,const size_t bits)
{
  int
    code;

  ssize_t
    i;

  size_t
    one;

  while (((lzw_info->code_info.bit+bits) > (8*lzw_info->code_info.count)) &&
         (lzw_info->code_info.eof == MagickFalse))
  {
    ssize_t
      count;