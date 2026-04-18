// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 76/94



  if (logging != MagickFalse)
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  Wrote PNG image data");

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    Width: %.20g",(double) ping_width);

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    Height: %.20g",(double) ping_height);

      if (mng_info->depth)
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Defined png:bit-depth: %d",mng_info->depth);
        }

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    PNG bit-depth written: %d",ping_bit_depth);

      if (mng_info->colortype)
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Defined png:color-type: %d",mng_info->colortype-1);
        }

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    PNG color-type written: %d",ping_color_type);

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    PNG Interlace method: %d",ping_interlace_method);
    }

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  Writing PNG end info");

  png_write_end(ping,ping_info);

  if (mng_info->need_fram != MagickFalse &&
      (int) image->dispose == BackgroundDispose)
    {
      if (mng_info->need_defi == MagickFalse &&
          (mng_info->page.x || mng_info->page.y ||
          (ping_width != mng_info->page.width) ||
          (ping_height != mng_info->page.height)))
        {
          unsigned char
            chunk[32];

          /*
            Write FRAM 4 with clipping boundaries followed by FRAM 1.
          */
          (void) WriteBlobMSBULong(image,27L);  /* data length=27 */
          PNGType(chunk,mng_FRAM);
          LogPNGChunk(logging,mng_FRAM,27L);
          chunk[4]=4;
          chunk[5]=0;  /* frame name separator (no name) */
          chunk[6]=1;  /* flag for changing delay, for next frame only */
          chunk[7]=0;  /* flag for changing frame timeout */
          chunk[8]=1;  /* flag for changing frame clipping for next frame */
          chunk[9]=0;  /* flag for changing frame sync_id */
          PNGLong(chunk+10,(png_uint_32) (0L)); /* temporary 0 delay */
          chunk[14]=0; /* clipping boundaries delta type */
          PNGLong(chunk+15,(png_uint_32) (mng_info->page.x)); /* left cb */
          PNGLong(chunk+19,
             (png_uint_32) (mng_info->page.x + ping_width));
          PNGLong(chunk+23,(png_uint_32) (mng_info->page.y)); /* top cb */
          PNGLong(chunk+27,
             (png_uint_32) (mng_info->page.y + ping_height));
          (void) WriteBlob(image,31,chunk);
          (void) WriteBlobMSBULong(image,crc32(0,chunk,31));
          mng_info->old_framing_mode=4;
          mng_info->framing_mode=1;
        }

      else
        mng_info->framing_mode=3;
    }
  if ((mng_info->write_mng != MagickFalse) &&
      (mng_info->need_fram == MagickFalse) && ((int) image->dispose == 3))
     png_error(ping, "Cannot convert GIF with disposal method 3 to MNG-LC");

  /*
    Free PNG resources.
  */

  png_destroy_write_struct(&ping,&ping_info);

  pixel_info=RelinquishVirtualMemory(pixel_info);

  if (ping_have_blob != MagickFalse)
     (void) CloseBlob(image);

  image_info=DestroyImageInfo(image_info);
  image=DestroyImage(image);

  /* Store bit depth actually written */
  s[0]=(char) ping_bit_depth;
  s[1]='\0';

  (void) SetImageProperty(IMimage,"png:bit-depth-written",s,exception);

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  exit WriteOnePNGImage()");

#ifdef IMPNG_SETJMP_NOT_THREAD_SAFE
  UnlockSemaphoreInfo(ping_semaphore);
#endif

   /* }  for navigation to beginning of SETJMP-protected block. Revert to
    *    Throwing an Exception when an error occurs.
    */

  return(MagickTrue);
/*  End write one PNG image */

}