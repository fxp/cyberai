// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/compress.c
// Segment 6/11



    /*
      Initialize scanline to white.
    */
    memset(scanline,0,sizeof(*scanline)*image->columns);
    /*
      Decode Huffman encoded scanline.
    */
    color=MagickTrue;
    code=0;
    count=0;
    length=0;
    runlength=0;
    x=0;
    for ( ; ; )
    {
      if (byte == EOF)
        break;
      if (x >= (ssize_t) image->columns)
        {
          while (runlength < 11)
            InputBit(bit);
          do { InputBit(bit); } while ((int) bit == 0);
          break;
        }
      bail=MagickFalse;
      do
      {
        if (runlength < 11)
          InputBit(bit)
        else
          {
            InputBit(bit);
            if ((int) bit != 0)
              {
                null_lines++;
                if (x != 0)
                  null_lines=0;
                bail=MagickTrue;
                break;
              }
          }
        code=(code << 1)+(size_t) bit;
        length++;
      } while (code == 0);
      if (bail != MagickFalse)
        break;
      if (length > 13)
        {
          while (runlength < 11)
            InputBit(bit);
          do { InputBit(bit); } while ((int) bit == 0);
          break;
        }
      if (color != MagickFalse)
        {
          if (length < 4)
            continue;
          entry=mw_hash[((length+MWHashA)*(code+MWHashB)) % HashSize];
        }
      else
        {
          if (length < 2)
            continue;
          entry=mb_hash[((length+MBHashA)*(code+MBHashB)) % HashSize];
        }
      if (entry == (const HuffmanTable *) NULL)
        continue;
      if ((entry->length != length) || (entry->code != code))
        continue;
      switch (entry->id)
      {
        case TWId:
        case TBId:
        {
          count+=(ssize_t) entry->count;
          if ((x+count) > (ssize_t) image->columns)
            count=(ssize_t) image->columns-x;
          if (count > 0)
            {
              if (color != MagickFalse)
                {
                  x+=count;
                  count=0;
                }
              else
                for ( ; count > 0; count--)
                  if ((x >= 0) && (x < (ssize_t) image->columns))
                    scanline[x++]=(unsigned char) 1;
            }
          color=(unsigned int)
            ((color == MagickFalse) ? MagickTrue : MagickFalse);
          break;
        }
        case MWId:
        case MBId:
        case EXId:
        {
          count+=(ssize_t) entry->count;
          break;
        }
        default:
          break;
      }
      code=0;
      length=0;
    }
    /*
      Transfer scanline to image pixels.
    */
    p=scanline;
    q=QueueCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      index=(Quantum) (*p++);
      SetPixelIndex(image,index,q);
      SetPixelViaPixelInfo(image,image->colormap+(ssize_t) index,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
    proceed=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
      image->rows);
    if (proceed == MagickFalse)
      break;
    y++;
  }
  image_view=DestroyCacheView(image_view);
  image->rows=(size_t) MagickMax((size_t) y-3,1);
  image->compression=FaxCompression;
  /*
    Free decoder memory.
  */
  mw_hash=(HuffmanTable **) RelinquishMagickMemory(mw_hash);
  mb_hash=(HuffmanTable **) RelinquishMagickMemory(mb_hash);
  scanline=(unsigned char *) RelinquishMagickMemory(scanline);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H u f f m a n E n c o d e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  HuffmanEncodeImage() compresses an image via Huffman-coding.
%
%  The format of the HuffmanEncodeImage method is:
%
%      MagickBooleanType HuffmanEncodeImage(const ImageInfo *image_info,
%        Image *image,Image *inject_image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info..
%
%    o image: the image.
%
%    o inject_image: inject into the image stream.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType HuffmanEncodeImage(const ImageInfo *image_info,
  Image *image,Image *inject_image,ExceptionInfo *exception)
{
#define HuffmanOutput