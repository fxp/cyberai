      if ( FT_List_Find( &loader->composites,
                         FT_UINT_TO_POINTER( glyph_index ) ) )
      {
        FT_TRACE1(( "TT_Load_Composite_Glyph:"
                    " infinite recursion detected\n" ));
        error = FT_THROW( Invalid_Composite );
        goto Exit;
      }

      else if ( node )
        node->data = FT_UINT_TO_POINTER( glyph_index );

      else
      {
        if ( FT_QNEW( node ) )
          goto Exit;
        node->data = FT_UINT_TO_POINTER( glyph_index );
        FT_List_Add( &loader->composites, node );
      }

      start_point   = gloader->base.outline.n_points;
      start_contour = gloader->base.outline.n_contours;

      /* for each subglyph, read composite header */
      error = face->read_composite_glyph( loader );
      if ( error )
        goto Exit;

      /* store the offset of instructions */
      ins_pos = loader->ins_pos;

      /* all data we need are read */
      face->forget_glyph_frame( loader );
      opened_frame = 0;

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT

      if ( FT_IS_NAMED_INSTANCE( FT_FACE( face ) ) ||
           FT_IS_VARIATION( FT_FACE( face ) )      )
      {
        FT_UShort    i, limit;
        FT_SubGlyph  subglyph;

        FT_Outline  outline = { 0, 0, NULL, NULL, NULL, 0 };
        FT_Vector*  unrounded = NULL;


        limit = (FT_UShort)gloader->current.num_subglyphs;

        /* construct an outline structure for              */
        /* communication with `TT_Vary_Apply_Glyph_Deltas' */
        if ( FT_QNEW_ARRAY( outline.points, limit + 4 ) ||
             FT_QNEW_ARRAY( outline.tags, limit )       ||
             FT_QNEW_ARRAY( outline.contours, limit )   ||
             FT_QNEW_ARRAY( unrounded, limit + 4 )      )
          goto Exit1;

        outline.n_contours = outline.n_points = limit;

        subglyph = gloader->current.subglyphs;

        for ( i = 0; i < limit; i++, subglyph++ )
        {
          /* applying deltas for anchor points doesn't make sense, */
          /* but we don't have to specially check this since       */
          /* unused delta values are zero anyways                  */
          outline.points[i].x = subglyph->arg1;
          outline.points[i].y = subglyph->arg2;
          outline.tags[i]     = ON_CURVE_POINT;
          outline.contours[i] = i;
        }

        outline.points[i++] = loader->pp1;
        outline.points[i++] = loader->pp2;
        outline.points[i++] = loader->pp3;
        outline.points[i  ] = loader->pp4;

        /* this call provides additional offsets */
        /* for each component's translation      */
        if ( FT_SET_ERROR( TT_Vary_Apply_Glyph_Deltas( loader,
                                                       &outline,
                                                       unrounded ) ) )
          goto Exit1;

        subglyph = gloader->current.subglyphs;

        for ( i = 0; i < limit; i++, subglyph++ )
        {
          if ( subglyph->flags & ARGS_ARE_XY_VALUES )
          {
            subglyph->arg1 = (FT_Int16)outline.points[i].x;
            subglyph->arg2 = (FT_Int16)outline.points[i].y;
          }
        }

      Exit1:
        FT_FREE( outline.points );
        FT_FREE( outline.tags );
        FT_FREE( outline.contours );
        FT_FREE( unrounded );

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

      /* if the flag FT_LOAD_NO_RECURSE is set, we return the subglyph */
      /* `as is' in the glyph slot (the client application will be     */
      /* responsible for interpreting these data)...                   */
      if ( loader->load_flags & FT_LOAD_NO_RECURSE )
      {
        FT_GlyphLoader_Add( gloader );
        loader->glyph->format = FT_GLYPH_FORMAT_COMPOSITE;

        goto Exit;
      }

      /*********************************************************************/
      /*********************************************************************/
      /*********************************************************************/

      {
