// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 27/94



  png_byte
    jng_color_type,
    jng_image_sample_depth,
    jng_image_compression_method,
    jng_image_interlace_method,
    jng_alpha_sample_depth,
    jng_alpha_compression_method,
    jng_alpha_filter_method,
    jng_alpha_interlace_method;

  const Quantum
    *s;

  ssize_t
    i,
    x;

  Quantum
    *q;

  unsigned char
    *p;

  unsigned int
    read_JSEP,
    reading_idat;

  size_t
    length;

  jng_alpha_compression_method=0;
  jng_alpha_sample_depth=8;
  jng_color_type=0;
  jng_height=0;
  jng_width=0;
  alpha_image=(Image *) NULL;
  color_image=(Image *) NULL;
  alpha_image_info=(ImageInfo *) NULL;
  color_image_info=(ImageInfo *) NULL;

  if (IsEventLogging() != MagickFalse)
    logging=LogMagickEvent(CoderEvent,GetMagickModule(),
      "  Enter ReadOneJNGImage()");

  image=mng_info->image;

  if (GetAuthenticPixelQueue(image) != (Quantum *) NULL)
    {
      /*
        Allocate next image structure.
      */
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
           "  AcquireNextImage()");

      AcquireNextImage(image_info,image,exception);

      if (GetNextImageInList(image) == (Image *) NULL)
        return(DestroyImageList(image));

      image=SyncNextImageInList(image);
    }
  mng_info->image=image;

  /*
    Signature bytes have already been read.
  */

  read_JSEP=MagickFalse;
  reading_idat=MagickFalse;
  for (;;)
  {
    char
      type[MagickPathExtent];

    unsigned char
      *chunk;

    unsigned int
      count;

    /*
      Read a new JNG chunk.
    */
    status=SetImageProgress(image,LoadImagesTag,TellBlob(image),
      2*GetBlobSize(image));

    if (status == MagickFalse)
      break;

    type[0]='\0';
    (void) ConcatenateMagickString(type,"errr",MagickPathExtent);
    length=(size_t) ReadBlobMSBLong(image);
    count=(unsigned int) ReadBlob(image,4,(unsigned char *) type);

    if (logging != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  Reading JNG chunk type %c%c%c%c, length: %.20g",
        type[0],type[1],type[2],type[3],(double) length);

    if (length > PNG_UINT_31_MAX || count == 0)
      {
        DestroyJNG(NULL,&color_image,&color_image_info,
          &alpha_image,&alpha_image_info);
        ThrowReaderException(CorruptImageError,"ImproperImageHeader");
      }
    if (length > GetBlobSize(image))
      {
        DestroyJNG(NULL,&color_image,&color_image_info,
          &alpha_image,&alpha_image_info);
        ThrowReaderException(CorruptImageError,
          "InsufficientImageDataInFile");
      }

    p=NULL;
    chunk=(unsigned char *) NULL;

    if (length != 0)
      {
        chunk=(unsigned char *) AcquireQuantumMemory(length,sizeof(*chunk));

        if (chunk == (unsigned char *) NULL)
          {
            DestroyJNG(NULL,&color_image,&color_image_info,
              &alpha_image,&alpha_image_info);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }

        for (i=0; i < (ssize_t) length; i++)
        {
          int
            c;

          c=ReadBlobByte(image);
          if (c == EOF)
            break;
          chunk[i]=(unsigned char) c;
        }
        for ( ; i < (ssize_t) length; i++)
          chunk[i]='\0';

        p=chunk;
      }

    (void) ReadBlobMSBLong(image);  /* read crc word */

    if (memcmp(type,mng_JHDR,4) == 0)
      {
        if (length == 16)
          {
            jng_width=(png_uint_32)mng_get_long(p);
            jng_height=(png_uint_32)mng_get_long(&p[4]);
            if ((jng_width == 0) || (jng_height == 0))
              {
                DestroyJNG(chunk,&color_image,&color_image_info,
                  &alpha_image,&alpha_image_info);
                ThrowReaderException(CorruptImageError,
                  "NegativeOrZeroImageSize");
              }
            jng_color_type=p[8];
            jng_image_sample_depth=p[9];
            jng_image_compression_method=p[10];
            jng_image_interlace_method=p[11];

            image->interlace=jng_image_interlace_method != 0 ? PNGInterlace :
              NoInterlace;

            jng_alpha_sample_depth=p[12];
            jng_alpha_compression_method=p[13];
            jng_alpha_filter_method=p[14];
            jng_alpha_interlace_method=p[15];

            if (logging != MagickFalse)
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "    jng_width:      %16lu,    jng_height:     %16lu\n"
                  "    jng_color_type: %16d,     jng_image_sample_depth: %3d\n"
                  "    jng_image_compression_method:%3d",
                  (unsigned long) jng_width, (unsigned long) jng_height,
                  jng_color_type, jng_image_sample_depth,
                  jng_image_compression_method);