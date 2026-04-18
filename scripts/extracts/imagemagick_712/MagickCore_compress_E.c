// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/compress.c
// Segment 5/11



  /*
    Allocate buffers.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->blob == (BlobInfo *) NULL)
    ThrowBinaryException(BlobError,"UnableToOpenBlob",image->filename);
  mb_hash=(HuffmanTable **) AcquireQuantumMemory(HashSize,sizeof(*mb_hash));
  mw_hash=(HuffmanTable **) AcquireQuantumMemory(HashSize,sizeof(*mw_hash));
  scanline=(unsigned char *) AcquireQuantumMemory((size_t) image->columns,
    sizeof(*scanline));
  if ((mb_hash == (HuffmanTable **) NULL) ||
      (mw_hash == (HuffmanTable **) NULL) ||
      (scanline == (unsigned char *) NULL))
    {
      if (mb_hash != (HuffmanTable **) NULL)
        mb_hash=(HuffmanTable **) RelinquishMagickMemory(mb_hash);
      if (mw_hash != (HuffmanTable **) NULL)
        mw_hash=(HuffmanTable **) RelinquishMagickMemory(mw_hash);
      if (scanline != (unsigned char *) NULL)
        scanline=(unsigned char *) RelinquishMagickMemory(scanline);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  /*
    Initialize Huffman tables.
  */
  for (i=0; i < HashSize; i++)
  {
    mb_hash[i]=(HuffmanTable *) NULL;
    mw_hash[i]=(HuffmanTable *) NULL;
  }
  InitializeHashTable(mw_hash,TWTable,MWHashA,MWHashB);
  InitializeHashTable(mw_hash,MWTable,MWHashA,MWHashB);
  InitializeHashTable(mw_hash,EXTable,MWHashA,MWHashB);
  InitializeHashTable(mb_hash,TBTable,MBHashA,MBHashB);
  InitializeHashTable(mb_hash,MBTable,MBHashA,MBHashB);
  InitializeHashTable(mb_hash,EXTable,MBHashA,MBHashB);
  /*
    Uncompress 1D Huffman to runlength encoded pixels.
  */
  byte=0;
  mask=0;
  null_lines=0;
  runlength=0;
  while (runlength < 11)
   InputBit(bit);
  do { InputBit(bit); } while ((int) bit == 0);
  image->resolution.x=204.0;
  image->resolution.y=196.0;
  image->units=PixelsPerInchResolution;
  image_view=AcquireAuthenticCacheView(image,exception);
  for (y=0; ((y < (ssize_t) image->rows) && (null_lines < 3)); )
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;