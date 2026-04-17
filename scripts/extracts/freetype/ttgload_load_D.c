        FT_UInt      n, num_base_points;
        FT_SubGlyph  subglyph       = NULL;

        FT_UInt      num_points     = start_point;
        FT_UInt      num_subglyphs  = gloader->current.num_subglyphs;
        FT_UInt      num_base_subgs = gloader->base.num_subglyphs;

        FT_Stream    old_stream     = loader->stream;
        FT_UInt      old_byte_len   = loader->byte_len;


        FT_GlyphLoader_Add( gloader );

        /* read each subglyph independently */
        for ( n = 0; n < num_subglyphs; n++ )
        {
          FT_Vector  pp[4];

          FT_Int  linear_hadvance;
          FT_Int  linear_vadvance;


          /* Each time we call `load_truetype_glyph' in this loop, the */
          /* value of `gloader.base.subglyphs' can change due to table */
          /* reallocations.  We thus need to recompute the subglyph    */
          /* pointer on each iteration.                                */
          subglyph = gloader->base.subglyphs + num_base_subgs + n;

          pp[0] = loader->pp1;
          pp[1] = loader->pp2;
          pp[2] = loader->pp3;
          pp[3] = loader->pp4;

          linear_hadvance = loader->linear;
          linear_vadvance = loader->vadvance;

          num_base_points = gloader->base.outline.n_points;

          error = load_truetype_glyph( loader,
                                       (FT_UInt)subglyph->index,
                                       recurse_count + 1,
                                       FALSE );
          if ( error )
            goto Exit;

          /* restore subglyph pointer */
          subglyph = gloader->base.subglyphs + num_base_subgs + n;

          /* restore phantom points if necessary */
          if ( !( subglyph->flags & USE_MY_METRICS ) )
          {
            loader->pp1 = pp[0];
            loader->pp2 = pp[1];
            loader->pp3 = pp[2];
            loader->pp4 = pp[3];

            loader->linear   = linear_hadvance;
            loader->vadvance = linear_vadvance;
          }

          num_points = gloader->base.outline.n_points;

          if ( num_points == num_base_points )
            continue;

          /* gloader->base.outline consists of three parts:           */
          /*                                                          */
          /* 0 ----> start_point ----> num_base_points ----> n_points */
          /*    (1)               (2)                   (3)           */
          /*                                                          */
          /* (1) points that exist from the beginning                 */
          /* (2) component points that have been loaded so far        */
          /* (3) points of the newly loaded component                 */
          error = TT_Process_Composite_Component( loader,
                                                  subglyph,
                                                  start_point,
                                                  num_base_points );
          if ( error )
            goto Exit;
        }

        loader->stream   = old_stream;
        loader->byte_len = old_byte_len;

        /* process the glyph */
        loader->ins_pos = ins_pos;
        if ( IS_HINTED( loader->load_flags ) &&
#ifdef TT_USE_BYTECODE_INTERPRETER
             subglyph                        &&
             subglyph->flags & WE_HAVE_INSTR &&
#endif
             num_points > start_point )
        {
          error = TT_Process_Composite_Glyph( loader,
                                              start_point,
                                              start_contour );
          if ( error )
            goto Exit;
        }
      }

      /* retain the overlap flag */
      if ( gloader->base.num_subglyphs                         &&
           gloader->base.subglyphs[0].flags & OVERLAP_COMPOUND )
        gloader->base.outline.flags |= FT_OUTLINE_OVERLAP;
    }

    /***********************************************************************/
    /***********************************************************************/
    /***********************************************************************/

  Exit:

    if ( opened_frame )
      face->forget_glyph_frame( loader );

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    if ( glyph_data_loaded )
      face->root.internal->incremental_interface->funcs->free_glyph_data(
        face->root.internal->incremental_interface->object,
        &glyph_data );

#endif

    return error;
  }


