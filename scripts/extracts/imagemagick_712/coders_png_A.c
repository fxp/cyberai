// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 33/94



  previous_fb.top=0;
  previous_fb.bottom=0;
  previous_fb.left=0;
  previous_fb.right=0;
  default_fb.top=0;
  default_fb.bottom=0;
  default_fb.left=0;
  default_fb.right=0;

  logging=LogMagickEvent(CoderEvent,GetMagickModule(),
    "  Enter ReadOneMNGImage()");

  image=mng_info->image;

  if (LocaleCompare(image_info->magick,"MNG") == 0)
    {
      char
        magic_number[MagickPathExtent];

      /* Verify MNG signature.  */
      count=(size_t) ReadBlob(image,8,(unsigned char *) magic_number);
      if ((count < 8) || (memcmp(magic_number,"\212MNG\r\n\032\n",8) != 0))
        ThrowReaderException(CorruptImageError,"ImproperImageHeader");

      /* Initialize some nonzero members of the MngReadInfo structure.  */
      for (i=0; i < MNG_MAX_OBJECTS; i++)
      {
        mng_info->object_clip[i].right=(ssize_t) PNG_UINT_31_MAX;
        mng_info->object_clip[i].bottom=(ssize_t) PNG_UINT_31_MAX;
      }
      mng_info->exists[0]=MagickTrue;
    }

  skipping_loop=(-1);
  first_mng_object=MagickTrue;
  mng_type=0;
  insert_layers=MagickFalse; /* should be False during convert or mogrify */
  default_frame_delay=0;
  default_frame_timeout=0;
  frame_delay=0;
  final_delay=1;
  mng_info->ticks_per_second=(unsigned long) image->ticks_per_second;
  object_id=0;
  skip_to_iend=MagickFalse;
  term_chunk_found=MagickFalse;
  mng_info->framing_mode=1;
  mandatory_back=MagickFalse;
  mng_background_color=image->background_color;
  default_fb=mng_info->frame;
  previous_fb=mng_info->frame;
  do
  {
    char
      type[MagickPathExtent] = "\0";

    if (LocaleCompare(image_info->magick,"MNG") == 0)
      {
        unsigned char
          *chunk;

        /*
          Read a new chunk.
        */
        type[0]='\0';
        (void) ConcatenateMagickString(type,"errr",MagickPathExtent);
        length=(size_t) ReadBlobMSBLong(image);
        count=(size_t) ReadBlob(image,4,(unsigned char *) type);

        if (logging != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
           "  Reading MNG chunk type %c%c%c%c, length: %.20g",
           type[0],type[1],type[2],type[3],(double) length);

        if ((length > PNG_UINT_31_MAX) || (length > GetBlobSize(image)) ||
            (count < 4))
          ThrowReaderException(CorruptImageError,"CorruptImage");

        p=NULL;
        chunk=(unsigned char *) NULL;
        if (length != 0)
          {
            chunk=(unsigned char *) AcquireQuantumMemory(length,sizeof(*chunk));
            if (chunk == (unsigned char *) NULL)
              ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
            if (ReadBlob(image,length,chunk) != (ssize_t) length)
              {
                chunk=(unsigned char *) RelinquishMagickMemory(chunk);
                ThrowReaderException(CorruptImageError,"CorruptImage");
              }
            p=chunk;
          }
        (void) ReadBlobMSBLong(image);  /* read crc word */

#if !defined(JNG_SUPPORTED)
        if (memcmp(type,mng_JHDR,4) == 0)
          {
            skip_to_iend=MagickTrue;

            if (mng_info->jhdr_warning == 0)
              (void) ThrowMagickException(exception,GetMagickModule(),
                CoderError,"JNGCompressNotSupported","`%s'",image->filename);

            mng_info->jhdr_warning++;
          }
#endif
        if (memcmp(type,mng_DHDR,4) == 0)
          {
            skip_to_iend=MagickTrue;

            if (mng_info->dhdr_warning == 0)
              (void) ThrowMagickException(exception,GetMagickModule(),
                CoderError,"DeltaPNGNotSupported","`%s'",image->filename);

            mng_info->dhdr_warning++;
          }
        if (memcmp(type,mng_MEND,4) == 0)
          {
            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            break;
          }

        if (skip_to_iend)
          {
            if (memcmp(type,mng_IEND,4) == 0)
              skip_to_iend=MagickFalse;

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);

            if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "  Skip to IEND.");

            continue;
          }

        if (memcmp(type,mng_MHDR,4) == 0)
          {
            if (length != 28)
              {
                chunk=(unsigned char *) RelinquishMagickMemory(chunk);
                ThrowReaderException(CorruptImageError,"CorruptImage");
              }

            mng_info->mng_width=(unsigned long)mng_get_long(p);
            mng_info->mng_height=(unsigned long)mng_get_long(&p[4]);

            if (logging != MagickFalse)
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  MNG width: %.20g",(double) mng_info->mng_width);
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  MNG height: %.20g",(double) mng_info->mng_height);
              }