          substreams[GLYPH_STREAM].offset = FT_STREAM_POS();
        }

        size_needed = 12 + composite_size + instruction_size;
        if ( glyph_buf_size < size_needed )
        {
          if ( FT_QREALLOC( glyph_buf, glyph_buf_size, size_needed ) )
            goto Fail;
          glyph_buf_size = size_needed;
        }

        pointer = glyph_buf + glyph_size;
        WRITE_USHORT( pointer, n_contours );
        glyph_size += 2;

        /* Read x_min for current glyph. */
        if ( FT_STREAM_SEEK( substreams[BBOX_STREAM].offset ) ||
             FT_READ_USHORT( x_min )                          )
          goto Fail;
        /* No increment here because we read again. */

        if ( FT_STREAM_SEEK( substreams[BBOX_STREAM].offset ) ||
             FT_STREAM_READ( glyph_buf + glyph_size, 8 )      )
          goto Fail;

        substreams[BBOX_STREAM].offset += 8;
        glyph_size                     += 8;

        if ( FT_STREAM_SEEK( substreams[COMPOSITE_STREAM].offset )    ||
             FT_STREAM_READ( glyph_buf + glyph_size, composite_size ) )
          goto Fail;

        substreams[COMPOSITE_STREAM].offset += composite_size;
        glyph_size                          += composite_size;

        if ( have_instructions )
        {
          pointer = glyph_buf + glyph_size;
          WRITE_USHORT( pointer, instruction_size );
          glyph_size += 2;

          if ( FT_STREAM_SEEK( substreams[INSTRUCTION_STREAM].offset )    ||
               FT_STREAM_READ( glyph_buf + glyph_size, instruction_size ) )
            goto Fail;

          substreams[INSTRUCTION_STREAM].offset += instruction_size;
          glyph_size                            += instruction_size;
        }
      }
      else if ( n_contours > 0 )
      {
        /* simple glyph */
        FT_ULong   total_n_points = 0;
        FT_UShort  n_points_contour;
        FT_UInt    j;
        FT_ULong   flag_size;
        FT_ULong   triplet_size;
        FT_ULong   triplet_bytes_used;
        FT_Bool    have_overlap  = FALSE;
        FT_Byte    overlap_bitmap;
        FT_ULong   overlap_offset;
        FT_Byte*   flags_buf     = NULL;
        FT_Byte*   triplet_buf   = NULL;
        FT_UShort  instruction_size;
        FT_ULong   size_needed;
        FT_Int     end_point;
        FT_UInt    contour_ix;

        FT_Byte*   pointer = NULL;


        /* Set `have_overlap`. */
        if ( overlap_bitmap_offset )
        {
          overlap_offset = overlap_bitmap_offset + ( i >> 3 );
          if ( FT_STREAM_SEEK( overlap_offset ) ||
               FT_READ_BYTE( overlap_bitmap )   )
            goto Fail;
          if ( overlap_bitmap & ( 0x80 >> ( i & 7 ) ) )
            have_overlap = TRUE;
        }

        if ( FT_QNEW_ARRAY( n_points_arr, n_contours ) )
          goto Fail;

        if ( FT_STREAM_SEEK( substreams[N_POINTS_STREAM].offset ) )
          goto Fail;

        for ( j = 0; j < n_contours; ++j )
        {
          if ( READ_255USHORT( n_points_contour ) )
            goto Fail;
          n_points_arr[j] = n_points_contour;
          /* Prevent negative/overflow. */
          if ( total_n_points + n_points_contour < total_n_points )
            goto Fail;
          total_n_points += n_points_contour;
        }
        substreams[N_POINTS_STREAM].offset = FT_STREAM_POS();

        flag_size = total_n_points;
        if ( flag_size > substreams[FLAG_STREAM].size )
          goto Fail;

        flags_buf   = stream->base + substreams[FLAG_STREAM].offset;
        triplet_buf = stream->base + substreams[GLYPH_STREAM].offset;

        if ( substreams[GLYPH_STREAM].size <
               ( substreams[GLYPH_STREAM].offset -
                 substreams[GLYPH_STREAM].start ) )
          goto Fail;

        triplet_size       = substreams[GLYPH_STREAM].size -
                               ( substreams[GLYPH_STREAM].offset -
                                 substreams[GLYPH_STREAM].start );
        triplet_bytes_used = 0;

        /* Create array to store point information. */
        points_size = total_n_points;
        if ( FT_QNEW_ARRAY( points, points_size ) )
          goto Fail;

        if ( triplet_decode( flags_buf,
                             triplet_buf,
                             triplet_size,
                             total_n_points,
                             points,
                             &triplet_bytes_used ) )
          goto Fail;

