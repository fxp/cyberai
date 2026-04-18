// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 5/18



  /*
    Allocate encoder tables.
  */
  assert(image != (Image *) NULL);
  one=1;
  packet=(unsigned char *) AcquireQuantumMemory(256,sizeof(*packet));
  hash_code=(short *) AcquireQuantumMemory(MaxHashTable,sizeof(*hash_code));
  hash_prefix=(short *) AcquireQuantumMemory(MaxHashTable,sizeof(*hash_prefix));
  hash_suffix=(unsigned char *) AcquireQuantumMemory(MaxHashTable,
    sizeof(*hash_suffix));
  if ((packet == (unsigned char *) NULL) || (hash_code == (short *) NULL) ||
      (hash_prefix == (short *) NULL) ||
      (hash_suffix == (unsigned char *) NULL))
    {
      if (packet != (unsigned char *) NULL)
        packet=(unsigned char *) RelinquishMagickMemory(packet);
      if (hash_code != (short *) NULL)
        hash_code=(short *) RelinquishMagickMemory(hash_code);
      if (hash_prefix != (short *) NULL)
        hash_prefix=(short *) RelinquishMagickMemory(hash_prefix);
      if (hash_suffix != (unsigned char *) NULL)
        hash_suffix=(unsigned char *) RelinquishMagickMemory(hash_suffix);
      return(MagickFalse);
    }
  /*
    Initialize GIF encoder.
  */
  (void) memset(packet,0,256*sizeof(*packet));
  (void) memset(hash_code,0,MaxHashTable*sizeof(*hash_code));
  (void) memset(hash_prefix,0,MaxHashTable*sizeof(*hash_prefix));
  (void) memset(hash_suffix,0,MaxHashTable*sizeof(*hash_suffix));
  number_bits=data_size;
  max_code=MaxCode(number_bits);
  clear_code=(size_t) ((short) one << (data_size-1));
  end_of_information_code=clear_code+1;
  free_code=clear_code+2;
  length=0;
  datum=0;
  bits=0;
  GIFOutputCode(clear_code);
  /*
    Encode pixels.
  */
  offset=0;
  pass=0;
  waiting_code=0;
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;