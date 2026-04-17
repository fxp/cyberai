/* ===== pngrutil.c [png_handle_PLTE] ===== */
/* Lines: 1111–1240 */
/* File: pngrutil.c — libpng 1.6.45 */

void /* PRIVATE */
png_handle_IEND(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_debug(1, "in png_handle_IEND");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0 ||
       (png_ptr->mode & PNG_HAVE_IDAT) == 0)
      png_chunk_error(png_ptr, "out of place");

   png_ptr->mode |= (PNG_AFTER_IDAT | PNG_HAVE_IEND);

   png_crc_finish(png_ptr, length);

   if (length != 0)
      png_chunk_benign_error(png_ptr, "invalid");

   PNG_UNUSED(info_ptr)
}

#ifdef PNG_READ_gAMA_SUPPORTED
void /* PRIVATE */
png_handle_gAMA(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_fixed_point igamma;
   png_byte buf[4];

   png_debug(1, "in png_handle_gAMA");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & (PNG_HAVE_IDAT|PNG_HAVE_PLTE)) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   if (length != 4)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "invalid");
      return;
   }

   png_crc_read(png_ptr, buf, 4);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   igamma = png_get_fixed_point(NULL, buf);

   png_colorspace_set_gamma(png_ptr, &png_ptr->colorspace, igamma);
   png_colorspace_sync(png_ptr, info_ptr);
}
#endif

#ifdef PNG_READ_sBIT_SUPPORTED
void /* PRIVATE */
png_handle_sBIT(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   unsigned int truelen, i;
   png_byte sample_depth;
   png_byte buf[4];

   png_debug(1, "in png_handle_sBIT");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & (PNG_HAVE_IDAT|PNG_HAVE_PLTE)) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_sBIT) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "duplicate");
      return;
   }

   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      truelen = 3;
      sample_depth = 8;
   }

   else
   {
      truelen = png_ptr->channels;
      sample_depth = png_ptr->bit_depth;
   }

   if (length != truelen || length > 4)
   {
      png_chunk_benign_error(png_ptr, "invalid");
      png_crc_finish(png_ptr, length);
      return;
   }

   buf[0] = buf[1] = buf[2] = buf[3] = sample_depth;
   png_crc_read(png_ptr, buf, truelen);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   for (i=0; i<truelen; ++i)
   {
      if (buf[i] == 0 || buf[i] > sample_depth)
      {
         png_chunk_benign_error(png_ptr, "invalid");
         return;
      }
   }

   if ((png_ptr->color_type & PNG_COLOR_MASK_COLOR) !
/* ... truncated ... */