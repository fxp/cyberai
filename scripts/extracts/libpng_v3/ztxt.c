/* ===== pngrutil.c [png_handle_zTXt] ===== */
/* Lines: 2680–2780 */
/* File: pngrutil.c — libpng 1.6.45 */

   if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
      png_ptr->mode |= PNG_AFTER_IDAT;

   /* Note, "length" is sufficient here; we won't be adding
    * a null terminator later.
    */
   buffer = png_read_buffer(png_ptr, length, 2/*silent*/);

   if (buffer == NULL)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of memory");
      return;
   }

   png_crc_read(png_ptr, buffer, length);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   /* TODO: also check that the keyword contents match the spec! */
   for (keyword_length = 0;
      keyword_length < length && buffer[keyword_length] != 0;
      ++keyword_length)
      /* Empty loop to find end of name */ ;

   if (keyword_length > 79 || keyword_length < 1)
      errmsg = "bad keyword";

   /* zTXt must have some LZ data after the keyword, although it may expand to
    * zero bytes; we need a '\0' at the end of the keyword, the compression type
    * then the LZ data:
    */
   else if (keyword_length + 3 > length)
      errmsg = "truncated";

   else if (buffer[keyword_length+1] != PNG_COMPRESSION_TYPE_BASE)
      errmsg = "unknown compression type";

   else
   {
      png_alloc_size_t uncompressed_length = PNG_SIZE_MAX;

      /* TODO: at present png_decompress_chunk imposes a single application
       * level memory limit, this should be split to different values for iCCP
       * and text chunks.
       */
      if (png_decompress_chunk(png_ptr, length, keyword_length+2,
          &uncompressed_length, 1/*terminate*/) == Z_STREAM_END)
      {
         png_text text;

         if (png_ptr->read_buffer == NULL)
           errmsg="Read failure in png_handle_zTXt";
         else
         {
            /* It worked; png_ptr->read_buffer now looks like a tEXt chunk
             * except for the extra compression type byte and the fact that
             * it isn't necessarily '\0' terminated.
             */
            buffer = png_ptr->read_buffer;
            buffer[uncompressed_length+(keyword_length+2)] = 0;

            text.compression = PNG_TEXT_COMPRESSION_zTXt;
            text.key = (png_charp)buffer;
            text.text = (png_charp)(buffer + keyword_length+2);
            text.text_length = uncompressed_length;
            text.itxt_length = 0;
            text.lang = NULL;
            text.lang_key = NULL;

            if (png_set_text_2(png_ptr, info_ptr, &text, 1) != 0)
               errmsg = "insufficient memory";
         }
      }

      else
         errmsg = png_ptr->zstream.msg;
   }

   if (errmsg != NULL)
      png_chunk_benign_error(png_ptr, errmsg);
}
#endif

#ifdef PNG_READ_iTXt_SUPPORTED
/* Note: this does not correctly handle chunks that are > 64K under DOS */
void /* PRIVATE */
png_handle_iTXt(png_structrp png_ptr, png_inforp
/* ... truncated ... */