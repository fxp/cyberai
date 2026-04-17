                           FALSE,
                           glyph_index,
                           &leftBearing,
                           &advanceX );
        sfnt->get_metrics( face,
                           TRUE,
                           glyph_index,
                           &topBearing,
                           &advanceY );

        glyph->linearHoriAdvance = advanceX;
        glyph->linearVertAdvance = advanceY;

        glyph->metrics.horiAdvance = FT_MulFix( advanceX, x_scale );
        glyph->metrics.vertAdvance = FT_MulFix( advanceY, y_scale );

        return error;
      }

      FT_TRACE3(( "Failed to load SVG glyph\n" ));
    }

    /* return immediately if we only want SVG glyphs */
    if ( load_flags & FT_LOAD_SVG_ONLY )
    {
      error = FT_THROW( Invalid_Argument );
      goto Exit;
    }

#endif /* FT_CONFIG_OPTION_SVG */

    error = tt_loader_init( &loader, size, glyph, load_flags, FALSE );
    if ( error )
      goto Exit;

    /* done if we are only interested in the `hdmx` advance */
    if ( load_flags & FT_LOAD_ADVANCE_ONLY         &&
         !( load_flags & FT_LOAD_VERTICAL_LAYOUT ) &&
         loader.widthp                             )
    {
      glyph->metrics.horiAdvance = loader.widthp[glyph_index] * 64;
      goto Done;
    }

    glyph->format        = FT_GLYPH_FORMAT_OUTLINE;
    glyph->num_subglyphs = 0;
    glyph->outline.flags = 0;

    /* main loading loop */
    error = load_truetype_glyph( &loader, glyph_index, 0, FALSE );
    if ( !error )
    {
      if ( glyph->format == FT_GLYPH_FORMAT_COMPOSITE )
      {
        glyph->num_subglyphs = loader.gloader->base.num_subglyphs;
        glyph->subglyphs     = loader.gloader->base.subglyphs;
      }
      else
      {
        glyph->outline        = loader.gloader->base.outline;
        glyph->outline.flags &= ~FT_OUTLINE_SINGLE_PASS;

        /* Translate array so that (0,0) is the glyph's origin.  Note  */
        /* that this behaviour is independent on the value of bit 1 of */
        /* the `flags' field in the `head' table -- at least major     */
        /* applications like Acroread indicate that.                   */
        if ( loader.pp1.x )
          FT_Outline_Translate( &glyph->outline, -loader.pp1.x, 0 );
      }

#ifdef TT_USE_BYTECODE_INTERPRETER

      if ( IS_HINTED( load_flags ) )
      {
        glyph->control_data = loader.exec->glyphIns;
        glyph->control_len  = loader.exec->glyphSize;

        if ( loader.exec->GS.scan_control )
        {
          /* convert scan conversion mode to FT_OUTLINE_XXX flags */
          switch ( loader.exec->GS.scan_type )
          {
          case 0: /* simple drop-outs including stubs */
            glyph->outline.flags |= FT_OUTLINE_INCLUDE_STUBS;
            break;
          case 1: /* simple drop-outs excluding stubs */
            /* nothing; it's the default rendering mode */
            break;
          case 4: /* smart drop-outs including stubs */
            glyph->outline.flags |= FT_OUTLINE_SMART_DROPOUTS |
                                    FT_OUTLINE_INCLUDE_STUBS;
            break;
          case 5: /* smart drop-outs excluding stubs  */
            glyph->outline.flags |= FT_OUTLINE_SMART_DROPOUTS;
            break;

          default: /* no drop-out control */
            glyph->outline.flags |= FT_OUTLINE_IGNORE_DROPOUTS;
            break;
          }
        }
        else
          glyph->outline.flags |= FT_OUTLINE_IGNORE_DROPOUTS;
      }

#endif /* TT_USE_BYTECODE_INTERPRETER */

      error = compute_glyph_metrics( &loader, glyph_index );
    }

    /* Set the `high precision' bit flag.                           */
    /* This is _critical_ to get correct output for monochrome      */
    /* TrueType glyphs at all sizes using the bytecode interpreter. */
    /*                                                              */
    if ( !( load_flags & FT_LOAD_NO_SCALE ) &&
         size->metrics->y_ppem < 24         )
      glyph->outline.flags |= FT_OUTLINE_HIGH_PRECISION;

    FT_TRACE1(( "  subglyphs = %u, contours = %hu, points = %hu,"
                " flags = 0x%.3x\n",
                loader.gloader->base.num_subglyphs,
                glyph->outline.n_contours,
                glyph->outline.n_points,
                glyph->outline.flags ));

  Done:
    tt_loader_done( &loader );

  Exit:
#ifdef FT_DEBUG_LEVEL_TRACE
    if ( error )
      FT_TRACE1(( "  failed (error code 0x%x)\n",
                  error ));
#endif

    return error;
  }


/* END */
