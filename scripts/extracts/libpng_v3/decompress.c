/* ===== pngrutil.c [png_decompress_chunk] ===== */
/* Lines: 622–760 */
/* File: pngrutil.c — libpng 1.6.45 */

png_decompress_chunk(png_structrp png_ptr,
    png_uint_32 chunklength, png_uint_32 prefix_size,
    png_alloc_size_t *newlength /* must be initialized to the maximum! */,
    int terminate /*add a '\0' to the end of the uncompressed data*/)
{
   /* TODO: implement different limits for different types of chunk.
    *
    * The caller supplies *newlength set to the maximum length of the
    * uncompressed data, but this routine allocates space for the prefix and
    * maybe a '\0' terminator too.  We have to assume that 'prefix_size' is
    * limited only by the maximum chunk size.
    */
   png_alloc_size_t limit = PNG_SIZE_MAX;

# ifdef PNG_SET_USER_LIMITS_SUPPORTED
   if (png_ptr->user_chunk_malloc_max > 0 &&
       png_ptr->user_chunk_malloc_max < limit)
      limit = png_ptr->user_chunk_malloc_max;
# elif PNG_USER_CHUNK_MALLOC_MAX > 0
   if (PNG_USER_CHUNK_MALLOC_MAX < limit)
      limit = PNG_USER_CHUNK_MALLOC_MAX;
# endif

   if (limit >= prefix_size + (terminate != 0))
   {
      int ret;

      limit -= prefix_size + (terminate != 0);

      if (limit < *newlength)
         *newlength = limit;

      /* Now try to claim the stream. */
      ret = png_inflate_claim(png_ptr, png_ptr->chunk_name);

      if (ret == Z_OK)
      {
         png_uint_32 lzsize = chunklength - prefix_size;

         ret = png_inflate(png_ptr, png_ptr->chunk_name, 1/*finish*/,
             /* input: */ png_ptr->read_buffer + prefix_size, &lzsize,
             /* output: */ NULL, newlength);

         if (ret == Z_STREAM_END)
         {
            /* Use 'inflateReset' here, not 'inflateReset2' because this
             * preserves the previously decided window size (otherwise it would
             * be necessary to store the previous window size.)  In practice
             * this doesn't matter anyway, because png_inflate will call inflate
             * with Z_FINISH in almost all cases, so the window will not be
             * maintained.
             */
            if (inflateReset(&png_ptr->zstream) == Z_OK)
            {
               /* Because of the limit checks above we know that the new,
                * expanded, size will fit in a size_t (let alone an
                * png_alloc_size_t).  Use png_malloc_base here to avoid an
                * extra OOM message.
                */
               png_alloc_size_t new_size = *newlength;
               png_alloc_size_t buffer_size = prefix_size + new_size +
                   (terminate != 0);
               png_bytep text = png_voidcast(png_bytep, png_malloc_base(png_ptr,
                   buffer_size));

               if (text != NULL)
               {
                  memset(text, 0, buffer_size);

                  ret = png_inflate(png_ptr, png_ptr->chunk_name, 1/*finish*/,
                      png
/* ... truncated ... */