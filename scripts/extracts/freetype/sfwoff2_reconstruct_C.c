        substreams[FLAG_STREAM].offset  += flag_size;
        substreams[GLYPH_STREAM].offset += triplet_bytes_used;

        if ( FT_STREAM_SEEK( substreams[GLYPH_STREAM].offset ) ||
             READ_255USHORT( instruction_size )                )
          goto Fail;

        substreams[GLYPH_STREAM].offset = FT_STREAM_POS();

        if ( total_n_points >= ( 1 << 27 ) )
          goto Fail;

        size_needed = 12 +
                      ( 2 * n_contours ) +
                      ( 5 * total_n_points ) +
                      instruction_size;
        if ( glyph_buf_size < size_needed )
        {
          if ( FT_QREALLOC( glyph_buf, glyph_buf_size, size_needed ) )
            goto Fail;
          glyph_buf_size = size_needed;
        }

        pointer = glyph_buf + glyph_size;
        WRITE_USHORT( pointer, n_contours );
        glyph_size += 2;

        if ( have_bbox )
        {
          /* Read x_min for current glyph. */
          if ( FT_STREAM_SEEK( substreams[BBOX_STREAM].offset ) ||
               FT_READ_USHORT( x_min )                          )
            goto Fail;
          /* No increment here because we read again. */

          if ( FT_STREAM_SEEK( substreams[BBOX_STREAM].offset ) ||
               FT_STREAM_READ( glyph_buf + glyph_size, 8 )      )
            goto Fail;
          substreams[BBOX_STREAM].offset += 8;
        }
        else
          compute_bbox( total_n_points, points, glyph_buf, &x_min );

        glyph_size = CONTOUR_OFFSET_END_POINT;

        pointer   = glyph_buf + glyph_size;
        end_point = -1;

        for ( contour_ix = 0; contour_ix < n_contours; ++contour_ix )
        {
          end_point += n_points_arr[contour_ix];
          if ( end_point >= 65536 )
            goto Fail;

          WRITE_SHORT( pointer, end_point );
          glyph_size += 2;
        }

        WRITE_USHORT( pointer, instruction_size );
        glyph_size += 2;

        if ( FT_STREAM_SEEK( substreams[INSTRUCTION_STREAM].offset )    ||
             FT_STREAM_READ( glyph_buf + glyph_size, instruction_size ) )
          goto Fail;

        substreams[INSTRUCTION_STREAM].offset += instruction_size;
        glyph_size                            += instruction_size;

        if ( store_points( total_n_points,
                           points,
                           n_contours,
                           instruction_size,
                           have_overlap,
                           glyph_buf,
                           glyph_buf_size,
                           &glyph_size ) )
          goto Fail;

        FT_FREE( points );
        FT_FREE( n_points_arr );
      }
      else
      {
        /* Empty glyph.          */
        /* Must not have a bbox. */
        if ( have_bbox )
        {
          FT_ERROR(( "Empty glyph has a bbox.\n" ));
          goto Fail;
        }
      }

      loca_values[i] = dest_offset - glyf_start;

      if ( WRITE_SFNT_BUF( glyph_buf, glyph_size ) )
        goto Fail;

      if ( pad4( &sfnt, sfnt_size, &dest_offset, memory ) )
        goto Fail;

      *glyf_checksum += compute_ULong_sum( glyph_buf, glyph_size );

      /* Store x_mins, may be required to reconstruct `hmtx'. */
      info->x_mins[i] = (FT_Short)x_min;
    }

    info->glyf_table->dst_length = dest_offset - info->glyf_table->dst_offset;
    info->loca_table->dst_offset = dest_offset;

    /* `loca[n]' will be equal to the length of the `glyf' table. */
    loca_values[num_glyphs] = info->glyf_table->dst_length;

    if ( store_loca( loca_values,
                     num_glyphs + 1,
                     index_format,
                     loca_checksum,
                     &sfnt,
                     sfnt_size,
                     &dest_offset,
                     memory ) )
      goto Fail;

    info->loca_table->dst_length = dest_offset - info->loca_table->dst_offset;

    FT_TRACE4(( "  loca table info:\n" ));
    FT_TRACE4(( "    dst_offset = %lu\n", info->loca_table->dst_offset ));
    FT_TRACE4(( "    dst_length = %lu\n", info->loca_table->dst_length ));
    FT_TRACE4(( "    checksum = %09lx\n", *loca_checksum ));

    /* Set pointer `sfnt_bytes' to its correct value. */
    *sfnt_bytes = sfnt;
    *out_offset = dest_offset;

    FT_FREE( substreams );
    FT_FREE( loca_values );
    FT_FREE( n_points_arr );
    FT_FREE( glyph_buf );
    FT_FREE( points );

    return error;

  Fail:
    if ( !error )
      error = FT_THROW( Invalid_Table );

    /* Set pointer `sfnt_bytes' to its correct value. */
    *sfnt_bytes = sfnt;

    FT_FREE( substreams );
    FT_FREE( loca_values );
    FT_FREE( n_points_arr );
    FT_FREE( glyph_buf );
    FT_FREE( points );

    return error;
  }


  /* Get `x_mins' for untransformed `glyf' table. */
  static FT_Error
  get_x_mins( FT_Stream     stream,
              WOFF2_Table*  tables,
              FT_UShort     num_tables,
              WOFF2_Info    info,
              FT_Memory     memory )
