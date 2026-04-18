// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 6/18



    p=GetVirtualPixels(image,0,offset,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    if (y == 0)
      {
        waiting_code=(short) GetPixelIndex(image,p);
        p+=(ptrdiff_t) GetPixelChannels(image);
      }
    for (x=(ssize_t) (y == 0 ? 1 : 0); x < (ssize_t) image->columns; x++)
    {
      /*
        Probe hash table.
      */
      next_pixel=MagickFalse;
      displacement=1;
      index=(Quantum) ((size_t) GetPixelIndex(image,p) & 0xff);
      p+=(ptrdiff_t) GetPixelChannels(image);
      k=(ssize_t) (((size_t) index << (MaxGIFBits-8))+(size_t) waiting_code);
      if (k >= MaxHashTable)
        k-=MaxHashTable;
      if (k < 0)
        continue;
      if (hash_code[k] > 0)
        {
          if ((hash_prefix[k] == waiting_code) &&
              (hash_suffix[k] == (unsigned char) index))
            {
              waiting_code=hash_code[k];
              continue;
            }
          if (k != 0)
            displacement=MaxHashTable-k;
          for ( ; ; )
          {
            k-=displacement;
            if (k < 0)
              k+=MaxHashTable;
            if (hash_code[k] == 0)
              break;
            if ((hash_prefix[k] == waiting_code) &&
                (hash_suffix[k] == (unsigned char) index))
              {
                waiting_code=hash_code[k];
                next_pixel=MagickTrue;
                break;
              }
          }
          if (next_pixel != MagickFalse)
            continue;
        }
      GIFOutputCode(waiting_code);
      if (free_code < MaxGIFTable)
        {
          hash_code[k]=(short) free_code++;
          hash_prefix[k]=waiting_code;
          hash_suffix[k]=(unsigned char) index;
        }
      else
        {
          /*
            Fill the hash table with empty entries.
          */
          for (k=0; k < MaxHashTable; k++)
            hash_code[k]=0;
          /*
            Reset compressor and issue a clear code.
          */
          free_code=clear_code+2;
          GIFOutputCode(clear_code);
          number_bits=data_size;
          max_code=MaxCode(number_bits);
        }
      waiting_code=(short) index;
    }
    if (image_info->interlace == NoInterlace)
      offset++;
    else
      switch (pass)
      {
        case 0:
        default:
        {
          offset+=8;
          if (offset >= (ssize_t) image->rows)
            {
              pass++;
              offset=4;
            }
          break;
        }
        case 1:
        {
          offset+=8;
          if (offset >= (ssize_t) image->rows)
            {
              pass++;
              offset=2;
            }
          break;
        }
        case 2:
        {
          offset+=4;
          if (offset >= (ssize_t) image->rows)
            {
              pass++;
              offset=1;
            }
          break;
        }
        case 3:
        {
          offset+=2;
          break;
        }
      }
  }
  /*
    Flush out the buffered code.
  */
  GIFOutputCode(waiting_code);
  GIFOutputCode(end_of_information_code);
  if (bits > 0)
    {
      /*
        Add a character to current packet.  Maximum packet size is 255.
      */
      packet[length++]=(unsigned char) (datum & 0xff);
      if (length == 255)
        {
          (void) WriteBlobByte(image,(unsigned char) length);
          (void) WriteBlob(image,length,packet);
          length=0;
        }
    }
  /*
    Flush accumulated data.
  */
  if (length > 0)
    {
      (void) WriteBlobByte(image,(unsigned char) length);
      (void) WriteBlob(image,length,packet);
    }
  /*
    Free encoder memory.
  */
  hash_suffix=(unsigned char *) RelinquishMagickMemory(hash_suffix);
  hash_prefix=(short *) RelinquishMagickMemory(hash_prefix);
  hash_code=(short *) RelinquishMagickMemory(hash_code);
  packet=(unsigned char *) RelinquishMagickMemory(packet);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s G I F                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsGIF() returns MagickTrue if the image format type, identified by the
%  magick string, is GIF.
%
%  The format of the IsGIF method is:
%
%      MagickBooleanType IsGIF(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: compare image format pattern against these bytes.
%
%    o length: Specifies the le