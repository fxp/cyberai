/* ===== EXTRACT: pngpread.c [push_read_chunk] ===== */
/* File: libpng 1.6.45 */

/* --- png_push_read_chunk + png_push_fill_buffer (L181-560) --- */
png_push_read_chunk(png_structrp png_ptr, png_inforp info_ptr)
{
   png_uint_32 chunk_name;
#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
   int keep; /* unknown handling method */
#endif

   /* First we make sure we have enough data for the 4-byte chunk name
    * and the 4-byte chunk length before proceeding with decoding the
    * chunk data.  To fully decode each of these chunks, we also make
    * sure we have enough data in the buffer for the 4-byte CRC at the
    * end of every chunk (except IDAT, which is handled separately).
    */
   if ((png_ptr->mode & PNG_HAVE_CHUNK_HEADER) == 0)
   {
      png_byte chunk_length[4];
      png_byte chunk_tag[4];

      PNG_PUSH_SAVE_BUFFER_IF_LT(8)
      png_push_fill_buffer(png_ptr, chunk_length, 4);
      png_ptr->push_length = png_get_uint_31(png_ptr, chunk_length);
      png_reset_crc(png_ptr);
      png_crc_read(png_ptr, chunk_tag, 4);
      png_ptr->chunk_name = PNG_CHUNK_FROM_STRING(chunk_tag);
      png_check_chunk_name(png_ptr, png_ptr->chunk_name);
      png_check_chunk_length(png_ptr, png_ptr->push_length);
      png_ptr->mode |= PNG_HAVE_CHUNK_HEADER;
   }

   chunk_name = png_ptr->chunk_name;

   if (chunk_name == png_IDAT)
   {
      if ((png_ptr->mode & PNG_AFTER_IDAT) != 0)
         png_ptr->mode |= PNG_HAVE_CHUNK_AFTER_IDAT;

      /* If we reach an IDAT chunk, this means we have read all of the
       * header chunks, and we can start reading the image (or if this
       * is called after the image has been read - we have an error).
       */
      if ((png_ptr->mode & PNG_HAVE_IHDR) == 0)
         png_error(png_ptr, "Missing IHDR before IDAT");

      else if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
          (png_ptr->mode & PNG_HAVE_PLTE) == 0)
         png_error(png_ptr, "Missing PLTE before IDAT");

      png_ptr->process_mode = PNG_READ_IDAT_MODE;

      if ((png_ptr->mode & PNG_HAVE_IDAT) != 0)
         if ((png_ptr->mode & PNG_HAVE_CHUNK_AFTER_IDAT) == 0)
            if (png_ptr->push_length == 0)
               return;

      png_ptr->mode |= PNG_HAVE_IDAT;

      if ((png_ptr->mode & PNG_AFTER_IDAT) != 0)
         png_benign_error(png_ptr, "Too many IDATs found");
   }

   if (chunk_name == png_IHDR)
   {
      if (png_ptr->push_length != 13)
         png_error(png_ptr, "Invalid IHDR length");

      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_IHDR(png_ptr, info_ptr, png_ptr->push_length);
   }

   else if (chunk_name == png_IEND)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_IEND(png_ptr, info_ptr, png_ptr->push_length);

      png_ptr->process_mode = PNG_READ_DONE_MODE;
      png_push_have_end(png_ptr, info_ptr);
   }

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
   else if ((keep = png_chunk_unknown_handling(png_ptr, chunk_name)) != 0)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_unknown(png_ptr, info_ptr, png_ptr->push_length, keep);

      if (chunk_name == png_PLTE)
         png_ptr->mode |= PNG_HAVE_PLTE;
   }
#endif

   else if (chunk_name == png_PLTE)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_PLTE(png_ptr, info_ptr, png_ptr->push_length);
   }

   else if (chunk_name == png_IDAT)
   {
      png_ptr->idat_size = png_ptr->push_length;
      png_ptr->process_mode = PNG_READ_IDAT_MODE;
      png_push_have_info(png_ptr, info_ptr);
      png_ptr->zstream.avail_out =
          (uInt) PNG_ROWBYTES(png_ptr->pixel_depth,
          png_ptr->iwidth) + 1;
      png_ptr->zstream.next_out = png_ptr->row_buf;
      return;
   }

#ifdef PNG_READ_gAMA_SUPPORTED
   else if (png_ptr->chunk_name == png_gAMA)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_gAMA(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_sBIT_SUPPORTED
   else if (png_ptr->chunk_name == png_sBIT)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_sBIT(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_cHRM_SUPPORTED
   else if (png_ptr->chunk_name == png_cHRM)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_cHRM(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_cICP_SUPPORTED
   else if (png_ptr->chunk_name == png_cICP)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_cICP(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_eXIf_SUPPORTED
   else if (png_ptr->chunk_name == png_eXIf)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_eXIf(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_sRGB_SUPPORTED
   else if (chunk_name == png_sRGB)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_sRGB(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_iCCP_SUPPORTED
   else if (png_ptr->chunk_name == png_iCCP)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_iCCP(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_sPLT_SUPPORTED
   else if (chunk_name == png_sPLT)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_sPLT(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_tRNS_SUPPORTED
   else if (chunk_name == png_tRNS)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_tRNS(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_bKGD_SUPPORTED
   else if (chunk_name == png_bKGD)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_bKGD(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_hIST_SUPPORTED
   else if (chunk_name == png_hIST)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_hIST(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_pHYs_SUPPORTED
   else if (chunk_name == png_pHYs)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_pHYs(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_oFFs_SUPPORTED
   else if (chunk_name == png_oFFs)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_oFFs(png_ptr, info_ptr, png_ptr->push_length);
   }
#endif

#ifdef PNG_READ_pCAL_SUPPORTED
   else if (chunk_name == png_pCAL)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_pCAL(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_sCAL_SUPPORTED
   else if (chunk_name == png_sCAL)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_sCAL(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_tIME_SUPPORTED
   else if (chunk_name == png_tIME)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_tIME(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_tEXt_SUPPORTED
   else if (chunk_name == png_tEXt)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_tEXt(png_ptr, info_ptr, png_ptr->push_length);
   }

#endif
#ifdef PNG_READ_zTXt_SUPPORTED
   else if (chunk_name == png_zTXt)
   {
      PNG_PUSH_SAVE_BUFFER_IF_FULL
      png_handle_zTXt(png_ptr, info_ptr, png_ptr->push_length);
  