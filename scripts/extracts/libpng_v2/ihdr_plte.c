/* ===== pngrutil.c [png_handle_IHDR+png_handle_PLTE] ===== */
/* Lines: 851–1050 */
/* File: pngrutil.c — libpng 1.6.45 */

png_handle_IHDR(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_byte buf[13];
   png_uint_32 width, height;
   int bit_depth, color_type, compression_type, filter_type;
   int interlace_type;

   png_debug(1, "in png_handle_IHDR");

   if ((png_ptr->mode & PNG_HAVE_IHDR) != 0)
      png_chunk_error(png_ptr, "out of place");

   /* Check the length */
   if (length != 13)
      png_chunk_error(png_ptr, "invalid");

   png_ptr->mode |= PNG_HAVE_IHDR;

   png_crc_read(png_ptr, buf, 13);
   png_crc_finish(png_ptr, 0);

   width = png_get_uint_31(png_ptr, buf);
   height = png_get_uint_31(png_ptr, buf + 4);
   bit_depth = buf[8];
   color_type = buf[9];
   compression_type = buf[10];
   filter_type = buf[11];
   interlace_type = buf[12];

   /* Set internal variables */
   png_ptr->width = width;
   png_ptr->height = height;
   png_ptr->bit_depth = (png_byte)bit_depth;
   png_ptr->interlaced = (png_byte)interlace_type;
   png_ptr->color_type = (png_byte)color_type;
#ifdef PNG_MNG_FEATURES_SUPPORTED
   png_ptr->filter_type = (png_byte)filter_type;
#endif
   png_ptr->compression_type = (png_byte)compression_type;

   /* Find number of channels */
   switch (png_ptr->color_type)
   {
      default: /* invalid, png_set_IHDR calls png_error */
      case PNG_COLOR_TYPE_GRAY:
      case PNG_COLOR_TYPE_PALETTE:
         png_ptr->channels = 1;
         break;

      case PNG_COLOR_TYPE_RGB:
         png_ptr->channels = 3;
         break;

      case PNG_COLOR_TYPE_GRAY_ALPHA:
         png_ptr->channels = 2;
         break;

      case PNG_COLOR_TYPE_RGB_ALPHA:
         png_ptr->channels = 4;
         break;
   }

   /* Set up other useful info */
   png_ptr->pixel_depth = (png_byte)(png_ptr->bit_depth * png_ptr->channels);
   png_ptr->rowbytes = PNG_ROWBYTES(png_ptr->pixel_depth, png_ptr->width);
   png_debug1(3, "bit_depth = %d", png_ptr->bit_depth);
   png_debug1(3, "channels = %d", png_ptr->channels);
   png_debug1(3, "rowbytes = %lu", (unsigned long)png_ptr->rowbytes);
   png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth,
       color_type, interlace_type, compression_type, filter_type);
}

/* Read and check the palette */
void /* PRIVATE */
png_handle_PLTE(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_color palette[PNG_MAX_PALETTE_LENGTH];
   int max_palette_length, num, i;
#ifdef PNG_POINTER_INDEXING_SUPPORTED
   png_colorp pal_ptr;
#endif

   png_debug(1, "in png_handle_PLTE");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   /* Moved to before the 'after IDAT' check below because otherwise duplicate
    * PLTE chunks are potentially ignored (the spec says there shall not be more
    * than one PLTE, the error is not treated as benign, so this check trumps
    * the requirement that PLTE appears before IDAT.)
    */
   else if ((png_ptr->mode & PNG_HAVE_PLTE) != 0)
      png_chunk_error(png_ptr, "duplicate");

   else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
   {
      /* This is benign because the non-benign error happened before, when an
       * IDAT was encountered in a color-mapped image with no PLTE.
       */
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   png_ptr->mode |= PNG_HAVE_PLTE;

   if ((png_ptr->color_type & PNG_COLOR_MASK_COLOR) == 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "ignored in grayscale PNG");
      return;
   }

#ifndef PNG_READ_OPT_PLTE_SUPPORTED
   if (png_ptr->color_type != PNG_COLOR_TYPE_PALETTE)
   {
      png_crc_finish(png_ptr, length);
      return;
   }
#endif

   if (length > 3*PNG_MAX_PALETTE_LENGTH || length % 3)
   {
      png_crc_finish(png_ptr, length);

      if (png_ptr->color_type != PNG_COLOR_TYPE_PALETTE)
         png_chunk_benign_error(png_ptr, "invalid");

      else
         png_chunk_error(png_ptr, "invalid");

      return;
   }

   /* The cast is safe because 'length' is less than 3*PNG_MAX_PALETTE_LENGTH */
   num = (int)length / 3;

   /* If the palette has 256 or fewer entries but is too large for the bit
    * depth, we don't issue an error, to preserve the 
/* ... truncated ... */