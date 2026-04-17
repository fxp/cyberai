/* ===== pngrutil.c [png_combine_row] ===== */
/* Lines: 3255–3430 */
/* File: pngrutil.c — libpng 1.6.45 */

png_combine_row(png_const_structrp png_ptr, png_bytep dp, int display)
{
   unsigned int pixel_depth = png_ptr->transformed_pixel_depth;
   png_const_bytep sp = png_ptr->row_buf + 1;
   png_alloc_size_t row_width = png_ptr->width;
   unsigned int pass = png_ptr->pass;
   png_bytep end_ptr = 0;
   png_byte end_byte = 0;
   unsigned int end_mask;

   png_debug(1, "in png_combine_row");

   /* Added in 1.5.6: it should not be possible to enter this routine until at
    * least one row has been read from the PNG data and transformed.
    */
   if (pixel_depth == 0)
      png_error(png_ptr, "internal row logic error");

   /* Added in 1.5.4: the pixel depth should match the information returned by
    * any call to png_read_update_info at this point.  Do not continue if we got
    * this wrong.
    */
   if (png_ptr->info_rowbytes != 0 && png_ptr->info_rowbytes !=
          PNG_ROWBYTES(pixel_depth, row_width))
      png_error(png_ptr, "internal row size calculation error");

   /* Don't expect this to ever happen: */
   if (row_width == 0)
      png_error(png_ptr, "internal row width error");

   /* Preserve the last byte in cases where only part of it will be overwritten,
    * the multiply below may overflow, we don't care because ANSI-C guarantees
    * we get the low bits.
    */
   end_mask = (pixel_depth * row_width) & 7;
   if (end_mask != 0)
   {
      /* end_ptr == NULL is a flag to say do nothing */
      end_ptr = dp + PNG_ROWBYTES(pixel_depth, row_width) - 1;
      end_byte = *end_ptr;
#     ifdef PNG_READ_PACKSWAP_SUPPORTED
      if ((png_ptr->transformations & PNG_PACKSWAP) != 0)
         /* little-endian byte */
         end_mask = (unsigned int)(0xff << end_mask);

      else /* big-endian byte */
#     endif
      end_mask = 0xff >> end_mask;
      /* end_mask is now the bits to *keep* from the destination row */
   }

   /* For non-interlaced images this reduces to a memcpy(). A memcpy()
    * will also happen if interlacing isn't supported or if the application
    * does not call png_set_interlace_handling().  In the latter cases the
    * caller just gets a sequence of the unexpanded rows from each interlace
    * pass.
    */
#ifdef PNG_READ_INTERLACING_SUPPORTED
   if (png_ptr->interlaced != 0 &&
       (png_ptr->transformations & PNG_INTERLACE) != 0 &&
       pass < 6 && (display == 0 ||
       /* The following copies everything for 'display' on passes 0, 2 and 4. */
       (display == 1 && (pass & 1) != 0)))
   {
      /* Narrow images may have no bits in a pass; the caller should handle
       * this, but this test is cheap:
       */
      if (row_width <= PNG_PASS_START_COL(pass))
         return;

      if (pixel_depth < 8)
      {
         /* For pixel depths up to 4 bpp the 8-pixel mask can be expanded to fit
          * in
/* ... truncated ... */