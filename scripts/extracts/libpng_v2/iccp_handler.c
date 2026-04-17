/* ===== pngrutil.c [png_handle_iCCP] ===== */
/* Lines: 1374–1600 */
/* File: pngrutil.c — libpng 1.6.45 */

png_handle_iCCP(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
/* Note: this does not properly handle profiles that are > 64K under DOS */
{
   png_const_charp errmsg = NULL; /* error message output, or no error */
   int finished = 0; /* crc checked */

   png_debug(1, "in png_handle_iCCP");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & (PNG_HAVE_IDAT|PNG_HAVE_PLTE)) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   /* Consistent with all the above colorspace handling an obviously *invalid*
    * chunk is just ignored, so does not invalidate the color space.  An
    * alternative is to set the 'invalid' flags at the start of this routine
    * and only clear them in they were not set before and all the tests pass.
    */

   /* The keyword must be at least one character and there is a
    * terminator (0) byte and the compression method byte, and the
    * 'zlib' datastream is at least 11 bytes.
    */
   if (length < 14)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "too short");
      return;
   }

   /* If a colorspace error has already been output skip this chunk */
   if ((png_ptr->colorspace.flags & PNG_COLORSPACE_INVALID) != 0)
   {
      png_crc_finish(png_ptr, length);
      return;
   }

   /* Only one sRGB or iCCP chunk is allowed, use the HAVE_INTENT flag to detect
    * this.
    */
   if ((png_ptr->colorspace.flags & PNG_COLORSPACE_HAVE_INTENT) == 0)
   {
      uInt read_length, keyword_length;
      char keyword[81];

      /* Find the keyword; the keyword plus separator and compression method
       * bytes can be at most 81 characters long.
       */
      read_length = 81; /* maximum */
      if (read_length > length)
         read_length = (uInt)length;

      png_crc_read(png_ptr, (png_bytep)keyword, read_length);
      length -= read_length;

      /* The minimum 'zlib' stream is assumed to be just the 2 byte header,
       * 5 bytes minimum 'deflate' stream, and the 4 byte checksum.
       */
      if (length < 11)
      {
         png_crc_finish(png_ptr, length);
         png_chunk_benign_error(png_ptr, "too short");
         return;
      }

      keyword_length = 0;
      while (keyword_length < 80 && keyword_length < read_length &&
         keyword[keyword_length] != 0)
         ++keyword_length;

      /* TODO: make the keyword checking common */
      if (keyword_length >= 1 && keyword_length <= 79)
      {
         /* We only understand '0' compression - deflate - so if we get a
          * different value we can't safely decode the chunk.
          */
         if (keyword_length+1 < read_length &&
            keyword[keyword_length+1] == PNG_COMPRESSION_TYPE_BASE)
         {
            read_length -= keyword_length+2;

            if (png_inflate_claim(png_ptr, png_iCCP) == Z_OK)
            {
               Byte profile_header[132]={0};
               Byte local_buffer[PNG_INFLATE_BUF_SIZE];
               png_alloc_size_t size = (sizeof profile_header);

               png_ptr->zstream.next_in = (Bytef*)keyword + (keyword_length+2);
               png_ptr->zstream.avail_in = read_length;
               (void)png_inflate_read(png_ptr, local_buffer,
                   (sizeof local_buffer), &length, profile_header, &size,
                   0/*finish: don't, because the output is too small*/);

               if (size == 0)
               {
                  /* We have the ICC profile header; do the basic header checks.
                   */
                  png_uint_32 profile_length = png_get_uint_32(profile_header);

                  if (png_icc_check_length(png_ptr, &png_ptr->colorspace,
                      keyword, profile_length) != 0)
                  {
                     /* The length is apparently ok, so we can check the 132
                      * byte header.
                      */
                     if (png_icc_check_header(png_ptr, &png_ptr->colorspace,
                         keyword, profile_length, profile_header,
                         png_ptr->color_type) 
/* ... truncated ... */