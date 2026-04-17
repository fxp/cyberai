/* ===== pngrutil.c [png_handle_pCAL] ===== */
/* Lines: 2302–2432 */
/* File: pngrutil.c — libpng 1.6.45 */

png_handle_pCAL(png_structrp png_ptr, png_inforp info_ptr, png_uint_32 length)
{
   png_int_32 X0, X1;
   png_byte type, nparams;
   png_bytep buffer, buf, units, endptr;
   png_charpp params;
   int i;

   png_debug(1, "in png_handle_pCAL");

   if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
      png_chunk_error(png_ptr, "missing IHDR");

   else if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of place");
      return;
   }

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_pCAL) != 0)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "duplicate");
      return;
   }

   png_debug1(2, "Allocating and reading pCAL chunk data (%u bytes)",
       length + 1);

   buffer = png_read_buffer(png_ptr, length+1, 2/*silent*/);

   if (buffer == NULL)
   {
      png_crc_finish(png_ptr, length);
      png_chunk_benign_error(png_ptr, "out of memory");
      return;
   }

   png_crc_read(png_ptr, buffer, length);

   if (png_crc_finish(png_ptr, 0) != 0)
      return;

   buffer[length] = 0; /* Null terminate the last string */

   png_debug(3, "Finding end of pCAL purpose string");
   for (buf = buffer; *buf; buf++)
      /* Empty loop */ ;

   endptr = buffer + length;

   /* We need to have at least 12 bytes after the purpose string
    * in order to get the parameter information.
    */
   if (endptr - buf <= 12)
   {
      png_chunk_benign_error(png_ptr, "invalid");
      return;
   }

   png_debug(3, "Reading pCAL X0, X1, type, nparams, and units");
   X0 = png_get_int_32((png_bytep)buf+1);
   X1 = png_get_int_32((png_bytep)buf+5);
   type = buf[9];
   nparams = buf[10];
   units = buf + 11;

   png_debug(3, "Checking pCAL equation type and number of parameters");
   /* Check that we have the right number of parameters for known
    * equation types.
    */
   if ((type == PNG_EQUATION_LINEAR && nparams != 2) ||
       (type == PNG_EQUATION_BASE_E && nparams != 3) ||
       (type == PNG_EQUATION_ARBITRARY && nparams != 3) ||
       (type == PNG_EQUATION_HYPERBOLIC && nparams != 4))
   {
      png_chunk_benign_error(png_ptr, "invalid parameter count");
      return;
   }

   else if (type >= PNG_EQUATION_LAST)
   {
      png_chunk_benign_error(png_ptr, "unrecognized equation type");
   }

   for (buf = units; *buf; buf++)
      /* Empty loop to move past the units string. */ ;

   png_debug(3, "Allocating pCAL parameters array");

   params = png_voidcast(png_charpp, png_malloc_warn(png_ptr,
       nparams * (sizeof (png_charp))));

   if (params == NULL)
   {
      png_chunk_benign_error(png_ptr, "out of memory");
      return;
   }

   /* Get pointers to the start of each parameter string. */
   for (i = 0; i < nparams; i++)
   {
      buf+
/* ... truncated ... */