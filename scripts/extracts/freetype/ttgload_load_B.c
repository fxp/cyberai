
      if ( FT_IS_NAMED_INSTANCE( FT_FACE( face ) ) ||
           FT_IS_VARIATION( FT_FACE( face ) )      )
      {
        /* a small outline structure with four elements for */
        /* communication with `TT_Vary_Apply_Glyph_Deltas'  */
        FT_Vector   points[4];
        FT_Outline  outline;

        /* unrounded values */
        FT_Vector  unrounded[4] = { {0, 0}, {0, 0}, {0, 0}, {0, 0} };


        points[0] = loader->pp1;
        points[1] = loader->pp2;
        points[2] = loader->pp3;
        points[3] = loader->pp4;

        outline.n_points   = 0;
        outline.n_contours = 0;
        outline.points     = points;
        outline.tags       = NULL;
        outline.contours   = NULL;

        /* this must be done before scaling */
        error = TT_Vary_Apply_Glyph_Deltas( loader,
                                            &outline,
                                            unrounded );
        if ( error )
          goto Exit;
      }

#endif /* TT_CONFIG_OPTION_GX_VAR_SUPPORT */

      /* scale phantom points, if necessary; */
      /* they get rounded in `TT_Hint_Glyph' */
      if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
      {
        loader->pp1.x = FT_MulFix( loader->pp1.x, x_scale );
        loader->pp2.x = FT_MulFix( loader->pp2.x, x_scale );
        /* pp1.y and pp2.y are always zero */

        loader->pp3.x = FT_MulFix( loader->pp3.x, x_scale );
        loader->pp3.y = FT_MulFix( loader->pp3.y, y_scale );
        loader->pp4.x = FT_MulFix( loader->pp4.x, x_scale );
        loader->pp4.y = FT_MulFix( loader->pp4.y, y_scale );
      }

      error = FT_Err_Ok;
      goto Exit;
    }

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    tt_get_metrics_incremental( loader, glyph_index );
#endif
    tt_loader_set_pp( loader );


    /***********************************************************************/
    /***********************************************************************/
    /***********************************************************************/

    /* we now open a frame again, right after the glyph header */
    /* (which consists of 10 bytes)                            */
    error = face->access_glyph_frame( loader, glyph_index,
                                      face->glyf_offset + offset + 10,
                                      loader->byte_len - 10 );
    if ( error )
      goto Exit;

    opened_frame = 1;

    /* if it is a simple glyph, load it */

    if ( loader->n_contours > 0 )
    {
      error = face->read_simple_glyph( loader );
      if ( error )
        goto Exit;

      /* all data have been read */
      face->forget_glyph_frame( loader );
      opened_frame = 0;

      error = TT_Process_Simple_Glyph( loader );
      if ( error )
        goto Exit;

      FT_GlyphLoader_Add( gloader );
    }

    /***********************************************************************/
    /***********************************************************************/
    /***********************************************************************/

    /* otherwise, load a composite! */
    else if ( loader->n_contours < 0 )
    {
      FT_Memory  memory = face->root.memory;

      FT_UInt   start_point;
      FT_UInt   start_contour;
      FT_ULong  ins_pos;  /* position of composite instructions, if any */

      FT_ListNode  node, node2;


      /* normalize the `n_contours' value */
      loader->n_contours = -1;

      /*
       * We store the glyph index directly in the `node->data' pointer,
       * following the glib solution (cf. macro `GUINT_TO_POINTER') with a
       * double cast to make this portable.  Note, however, that this needs
       * pointers with a width of at least 32 bits.
       */

      /* clear the nodes filled by sibling chains */
      node = ft_list_get_node_at( &loader->composites, recurse_count );
      for ( node2 = node; node2; node2 = node2->next )
        node2->data = (void*)-1;

      /* check whether we already have a composite glyph with this index */
