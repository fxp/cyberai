/* ===== EXTRACT: pngrutil.c [combine_row+do_read_interlace] ===== */
/* File: libpng 1.6.45 */

/* --- png_combine_row + png_do_read_interlace (L3255-3740) --- */
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
          * into 32 bits, then a single loop over the bytes using the four byte
          * values in the 32-bit mask can be used.  For the 'display' option the
          * expanded mask may also not require any masking within a byte.  To
          * make this work the PACKSWAP option must be taken into account - it
          * simply requires the pixels to be reversed in each byte.
          *
          * The 'regular' case requires a mask for each of the first 6 passes,
          * the 'display' case does a copy for the even passes in the range
          * 0..6.  This has already been handled in the test above.
          *
          * The masks are arranged as four bytes with the first byte to use in
          * the lowest bits (little-endian) regardless of the order (PACKSWAP or
          * not) of the pixels in each byte.
          *
          * NOTE: the whole of this logic depends on the caller of this function
          * only calling it on rows appropriate to the pass.  This function only
          * understands the 'x' logic; the 'y' logic is handled by the caller.
          *
          * The following defines allow generation of compile time constant bit
          * masks for each pixel depth and each possibility of swapped or not
          * swapped bytes.  Pass 'p' is in the range 0..6; 'x', a pixel index,
          * is in the range 0..7; and the result is 1 if the pixel is to be
          * copied in the pass, 0 if not.  'S' is for the sparkle method, 'B'
          * for the block method.
          *
          * With some compilers a compile time expression of the general form:
          *
          *    (shift >= 32) ? (a >> (shift-32)) : (b >> shift)
          *
          * Produces warnings with values of 'shift' in the range 33 to 63
          * because the right hand side of the ?: expression is evaluated by
          * the compiler even though it isn't used.  Microsoft Visual C (various
          * versions) and the Intel C compiler are known to do this.  To avoid
          * this the following macros are used in 1.5.6.  This is a temporary
          * solution to avoid destabilizing the code during the release process.
          */
#        if PNG_USE_COMPILE_TIME_MASKS
#           define PNG_LSR(x,s) ((x)>>((s) & 0x1f))
#           define PNG_LSL(x,s) ((x)<<((s) & 0x1f))
#        else
#           define PNG_LSR(x,s) ((x)>>(s))
#           define PNG_LSL(x,s) ((x)<<(s))
#        endif
#        define S_COPY(p,x) (((p)<4 ? PNG_LSR(0x80088822,(3-(p))*8+(7-(x))) :\
           PNG_LSR(0xaa55ff00,(7-(p))*8+(7-(x)))) & 1)
#        define B_COPY(p,x) (((p)<4 ? PNG_LSR(0xff0fff33,(3-(p))*8+(7-(x))) :\
           PNG_LSR(0xff55ff00,(7-(p))*8+(7-(x)))) & 1)

         /* Return a mask for pass 'p' pixel 'x' at depth 'd'.  The mask is
          * little endian - the first pixel is at bit 0 - however the extra
          * parameter 's' can be set to cause the mask position to be swapped
          * within each byte, to match the PNG format.  This is done by XOR of
          * the shift with 7, 6 or 4 for bit depths 1, 2 and 4.
          */
#        define PIXEL_MASK(p,x,d,s) \
            (PNG_LSL(((PNG_LSL(1U,(d)))-1),(((x)*(d))^((s)?8-(d):0))))

         /* Hence generate the appropriate 'block' or 'sparkle' pixel copy mask.
          */
#        define S_MASKx(p,x,d,s) (S_COPY(p,x)?PIXEL_MASK(p,x,d,s):0)
#        define B_MASKx(p,x,d,s) (B_COPY(p,x)?PIXEL_MASK(p,x,d,s):0)

         /* Combine 8 of these to get the full mask.  For the 1-bpp and 2-bpp
          * cases the result needs replicating, for the 4-bpp case the above
          * generates a full 32 bits.
          */
#        define MASK_EXPAND(m,d) ((m)*((d)==1?0x01010101:((d)==2?0x00010001:1)))

#        define S_MASK(p,d,s) MASK_EXPAND(S_MASKx(p,0,d,s) + S_MASKx(p,1,d,s) +\
            S_MASKx(p,2,d,s) + S_MASKx(p,3,d,s) + S_MASKx(p,4,d,s) +\
            S_MASKx(p,5,d,s) + S_MASKx(p,6,d,s) + S_MASKx(p,7,d,s), d)

#        define B_MASK(p,d,s) MASK_EXPAND(B_MASKx(p,0,d,s) + B_MASKx(p,1,d,s) +\
           