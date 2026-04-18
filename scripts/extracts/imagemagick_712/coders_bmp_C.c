// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 3/24



    if ((p < pixels) || (p >= q))
      break;
    count=ReadBlobByte(image);
    if (count == EOF)
      break;
    if (count > 0)
      {
        /*
          Encoded mode.
        */
        count=(int) MagickMin((ssize_t) count,(ssize_t) (q-p));
        byte=ReadBlobByte(image);
        if (byte == EOF)
          break;
        if (compression == BI_RLE8)
          {
            for (i=0; i < (ssize_t) count; i++)
              *p++=(unsigned char) byte;
          }
        else
          {
            for (i=0; i < (ssize_t) count; i++)
              *p++=(unsigned char)
                ((i & 0x01) != 0 ? (byte & 0x0f) : ((byte >> 4) & 0x0f));
          }
        x+=count;
      }
    else
      {
        /*
          Escape mode.
        */
        count=ReadBlobByte(image);
        if (count == EOF)
          break;
        if (count == 0x01)
          break;
        switch (count)
        {
          case 0x00:
          {
            /*
              End of line.
            */
            x=0;
            y++;
            p=pixels+y*(ssize_t) image->columns;
            break;
          }
          case 0x02:
          {
            /*
              Delta mode.
            */
            byte=ReadBlobByte(image);
            if (byte == EOF)
              return(MagickFalse);
            x+=byte;
            byte=ReadBlobByte(image);
            if (byte == EOF)
              return(MagickFalse);
            y+=byte;
            p=pixels+y*(ssize_t) image->columns+x;
            break;
          }
          default:
          {
            /*
              Absolute mode.
            */
            count=(int) MagickMin((ssize_t) count,(ssize_t) (q-p));
            if (count < 0)
              break;
            if (compression == BI_RLE8)
              for (i=0; i < (ssize_t) count; i++)
              {
                byte=ReadBlobByte(image);
                if (byte == EOF)
                  break;
                *p++=(unsigned char) byte;
              }
            else
              for (i=0; i < (ssize_t) count; i++)
              {
                if ((i & 0x01) == 0)
                  {
                    byte=ReadBlobByte(image);
                    if (byte == EOF)
                      break;
                  }
                *p++=(unsigned char)
                  ((i & 0x01) != 0 ? (byte & 0x0f) : ((byte >> 4) & 0x0f));
              }
            x+=count;
            /*
              Read pad byte.
            */
            if (compression == BI_RLE8)
              {
                if ((count & 0x01) != 0)
                  if (ReadBlobByte(image) == EOF)
                    break;
              }
            else
              if (((count & 0x03) == 1) || ((count & 0x03) == 2))
                if (ReadBlobByte(image) == EOF)
                  break;
            break;
          }
        }
      }
    status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
      image->rows);
    if (status == MagickFalse)
      break;
  }
  (void) ReadBlobByte(image);  /* end of line */
  (void) ReadBlobByte(image);
  return((p-pixels) < (ssize_t) number_pixels ? MagickFalse : MagickTrue);
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
%  EncodeImage compresses pixels using a runlength encoded format.
%
%  The format of the EncodeImage method is:
%
%    static MagickBooleanType EncodeImage(Image *image,
%      const size_t bytes_per_line,const unsigned char *pixels,
%      unsigned char *compressed_pixels)
%
%  A description of each parameter follows:
%
%    o image:  The image.
%
%    o bytes_per_line: the number of bytes in a scanline of compressed pixels
%
%    o pixels:  The address of a byte (8 bits) array of pixel data created by
%      the compression process.
%
%    o compressed_pixels:  The address of a byte (8 bits) array of compressed
%      pixel data.
%
*/
static size_t EncodeImage(Image *image,const size_t bytes_per_line,
  const unsigned char *pixels,unsigned char *compressed_pixels)
{
  MagickBooleanType
    status;

  const unsigned char
    *p;

  ssize_t
    i,
    x;

  unsigned char
    *q;

  ssize_t
    y;