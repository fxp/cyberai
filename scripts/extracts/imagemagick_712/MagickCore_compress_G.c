// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/compress.c
// Segment 7/11

Code(entry)  \
{  \
  mask=one << (entry->length-1);  \
  while (mask != 0)  \
  {  \
    OutputBit(((entry->code & mask) != 0 ? 1 : 0));  \
    mask>>=1;  \
  }  \
}

#define OutputBit(count)  \
{  \
DisableMSCWarning(4127) \
  if (count > 0)  \
    byte=byte | bit;  \
RestoreMSCWarning \
  bit>>=1;  \
  if ((int) (bit & 0xff) == 0)   \
    {  \
      if (LocaleCompare(image_info->magick,"FAX") == 0) \
        (void) WriteBlobByte(image,(unsigned char) byte);  \
      else \
        Ascii85Encode(image,byte); \
      byte='\0';  \
      bit=(unsigned char) 0x80;  \
    }  \
}

  const HuffmanTable
    *entry;

  int
    k,
    runlength;

  Image
    *huffman_image;

  MagickBooleanType
    proceed;

  ssize_t
    i,
    x;

  const Quantum
    *p;

  unsigned char
    *q;

  size_t
    mask,
    one,
    width;

  ssize_t
    n,
    y;

  unsigned char
    byte,
    bit,
    *scanline;

  /*
    Allocate scanline buffer.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(inject_image != (Image *) NULL);
  assert(inject_image->signature == MagickCoreSignature);
  one=1;
  width=inject_image->columns;
  if (LocaleCompare(image_info->magick,"FAX") == 0)
    width=(size_t) MagickMax(inject_image->columns,1728);
  scanline=(unsigned char *) AcquireQuantumMemory((size_t) width+1UL,
    sizeof(*scanline));
  if (scanline == (unsigned char *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      inject_image->filename);
  (void) memset(scanline,0,width*sizeof(*scanline));
  huffman_image=CloneImage(inject_image,0,0,MagickTrue,exception);
  if (huffman_image == (Image *) NULL)
    {
      scanline=(unsigned char *) RelinquishMagickMemory(scanline);
      return(MagickFalse);
    }
  (void) SetImageType(huffman_image,BilevelType,exception);
  byte='\0';
  bit=(unsigned char) 0x80;
  if (LocaleCompare(image_info->magick,"FAX") != 0)
    Ascii85Initialize(image);
  else
    {
      /*
        End of line.
      */
      for (k=0; k < 11; k++)
        OutputBit(0);
      OutputBit(1);
    }
  /*
    Compress to 1D Huffman pixels.
  */
  q=scanline;
  for (y=0; y < (ssize_t) huffman_image->rows; y++)
  {
    p=GetVirtualPixels(huffman_image,0,y,huffman_image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) huffman_image->columns; x++)
    {
      *q++=(unsigned char) (GetPixelIntensity(huffman_image,p) >=
        ((double) QuantumRange/2.0) ? 0 : 1);
      p+=(ptrdiff_t) GetPixelChannels(huffman_image);
    }
    /*
      Huffman encode scanline.
    */
    q=scanline;
    for (n=(ssize_t) width; n > 0; )
    {
      /*
        Output white run.
      */
      for (runlength=0; ((n > 0) && (*q == 0)); n--)
      {
        q++;
        runlength++;
      }
      if (runlength >= 64)
        {
          if (runlength < 1792)
            entry=MWTable+((runlength/64)-1);
          else
            entry=EXTable+(MagickMin((size_t) runlength,2560)-1792)/64;
          runlength-=(long) entry->count;
          HuffmanOutputCode(entry);
        }
      entry=TWTable+MagickMin((size_t) runlength,63);
      HuffmanOutputCode(entry);
      if (n != 0)
        {
          /*
            Output black run.
          */
          for (runlength=0; ((*q != 0) && (n > 0)); n--)
          {
            q++;
            runlength++;
          }
          if (runlength >= 64)
            {
              entry=MBTable+((runlength/64)-1);
              if (runlength >= 1792)
                entry=EXTable+(MagickMin((size_t) runlength,2560)-1792)/64;
              runlength-=(long) entry->count;
              HuffmanOutputCode(entry);
            }
          entry=TBTable+MagickMin((size_t) runlength,63);
          HuffmanOutputCode(entry);
        }
    }
    /*
      End of line.
    */
    for (k=0; k < 11; k++)
      OutputBit(0);
    OutputBit(1);
    q=scanline;
    if (GetPreviousImageInList(huffman_image) == (Image *) NULL)
      {
        proceed=SetImageProgress(huffman_image,LoadImageTag,
          (MagickOffsetType) y,huffman_image->rows);
        if (proceed == MagickFalse)
          break;
      }
  }
  /*
    End of page.
  */
  for (i=0; i < 6; i++)
  {
    for (k=0; k < 11; k++)
      OutputBit(0);
    OutputBit(1);
  }
  /*
    Flush bits.
  */
  if (((int) bit != 0x80) != 0)
    {
      if (LocaleCompare(image_info->magick,"FAX") == 0)
        (void) WriteBlobByte(image,byte);
      else
        Ascii85Encode(image,byte);
    }
  if (LocaleCompare(image_info->magick,"FAX") != 0)
    Ascii85Flush(image);
  huffman_image=DestroyImage(huffman_image);
  scanline=(unsigned char *) RelinquishMagickMemory(scanline);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%