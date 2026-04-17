/* ===== pngrutil.c [png_inflate] ===== */
/* Lines: 497–640 */
/* File: pngrutil.c — libpng 1.6.45 */

png_inflate(png_structrp png_ptr, png_uint_32 owner, int finish,
    /* INPUT: */ png_const_bytep input, png_uint_32p input_size_ptr,
    /* OUTPUT: */ png_bytep output, png_alloc_size_t *output_size_ptr)
{
   if (png_ptr->zowner == owner) /* Else not claimed */
   {
      int ret;
      png_alloc_size_t avail_out = *output_size_ptr;
      png_uint_32 avail_in = *input_size_ptr;

      /* zlib can't necessarily handle more than 65535 bytes at once (i.e. it
       * can't even necessarily handle 65536 bytes) because the type uInt is
       * "16 bits or more".  Consequently it is necessary to chunk the input to
       * zlib.  This code uses ZLIB_IO_MAX, from pngpriv.h, as the maximum (the
       * maximum value that can be stored in a uInt.)  It is possible to set
       * ZLIB_IO_MAX to a lower value in pngpriv.h and this may sometimes have
       * a performance advantage, because it reduces the amount of data accessed
       * at each step and that may give the OS more time to page it in.
       */
      png_ptr->zstream.next_in = PNGZ_INPUT_CAST(input);
      /* avail_in and avail_out are set below from 'size' */
      png_ptr->zstream.avail_in = 0;
      png_ptr->zstream.avail_out = 0;

      /* Read directly into the output if it is available (this is set to
       * a local buffer below if output is NULL).
       */
      if (output != NULL)
         png_ptr->zstream.next_out = output;

      do
      {
         uInt avail;
         Byte local_buffer[PNG_INFLATE_BUF_SIZE];

         /* zlib INPUT BUFFER */
         /* The setting of 'avail_in' used to be outside the loop; by setting it
          * inside it is possible to chunk the input to zlib and simply rely on
          * zlib to advance the 'next_in' pointer.  This allows arbitrary
          * amounts of data to be passed through zlib at the unavoidable cost of
          * requiring a window save (memcpy of up to 32768 output bytes)
          * every ZLIB_IO_MAX input bytes.
          */
         avail_in += png_ptr->zstream.avail_in; /* not consumed last time */

         avail = ZLIB_IO_MAX;

         if (avail_in < avail)
            avail = (uInt)avail_in; /* safe: < than ZLIB_IO_MAX */

         avail_in -= avail;
         png_ptr->zstream.avail_in = avail;

         /* zlib OUTPUT BUFFER */
         avail_out += png_ptr->zstream.avail_out; /* not written last time */

         avail = ZLIB_IO_MAX; /* maximum zlib can process */

         if (output == NULL)
         {
            /* Reset the output buffer each time round if output is NULL and
             * make available the full buffer, up to 'remaining_space'
             */
            png_ptr->zstream.next_out = local_buffer;
            if ((sizeof local_buffer) < avail)
               avail = (sizeof local_buffer);
         
/* ... truncated ... */