/* ===== pngrutil.c [png_handle_iTXt] ===== */
/* Lines: 2780–2917 */
/* File: pngrutil.c — libpng 1.6.45 */

      {
         png_crc_finish(png_ptr, length);
         return;
      }

      if (--png_ptr->user_chunk_cache_max == 1)
      {
         png_crc_finish(png_ptr, length);
         png_chunk_benign_error(png_ptr, "no space in chunk cache");
         return;
      }
   }
#endif

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
      png_ptr->mode |= PNG_AFTER_IDAT;

   buffer = png_read_buffer(png_ptr, length+1, 1/*warn*/);

   if (buffer == NULL)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of memory");
      return;
   }

   png_crc_read(png_ptr, buffer, length);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   /* First the keyword. */
   for (prefix_length=0;
      prefix_length < length && buffer[prefix_length] != 0;
      ++prefix_length)
      /* Empty loop */ ;

   /* Perform a basic check on the keyword length here. */
   if (prefix_length > 79 || prefix_length < 1)
      errmsg = "bad keyword";

   /* Expect keyword, compression flag, compression type, language, translated
    * keyword (both may be empty but are 0 terminated) then the text, which may
    * be empty.
    */
   else if (prefix_length + 5 > length)
      errmsg = "truncated";

   else if (buffer[prefix_length+1] == 0 ||
      (buffer[prefix_length+1] == 1 &&
      buffer[prefix_length+2] == PNG_COMPRESSION_TYPE_BASE))
   {
      int compressed = buffer[prefix_length+1] != 0;
      png_uint_32 language_offset, translated_keyword_offset;
      png_alloc_size_t uncompressed_length = 0;

      /* Now the language tag */
      prefix_length += 3;
      language_offset = prefix_length;

      for (; prefix_length < length && buffer[prefix_length] != 0;
         ++prefix_length)
         /* Empty loop */ ;

      /* WARNING: the length may be invalid here, this is checked below. */
      translated_keyword_offset = ++prefix_length;

      for (; prefix_length < length && buffer[prefix_length] != 0;
         ++prefix_length)
         /* Empty loop */ ;

      /* prefix_length should now be at the trailing '\0' of the translated
       * keyword, but it may already be over the end.  None of this arithmetic
       * can overflow because chunks are at most 2^31 bytes long, but on 16-bit
       * systems the available allocation may overflow.
       */
      ++prefix_length;

      if (compressed == 0 && prefix_length <= length)
         uncompressed_length = length - prefix_length;

      else if (compressed != 0 && prefix_length < length)
      {
         uncompressed_length = PNG_SIZE_MAX;

         /* TODO: at present png_decompress_chunk imposes a single application
          * level memory limit, this should be split to different values for
 
/* ... truncated ... */