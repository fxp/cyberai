                                  FT_SubGlyph  subglyph,
                                  FT_UInt      start_point,
                                  FT_UInt      num_base_points )
  {
    FT_GlyphLoader  gloader = loader->gloader;
    FT_Outline      current;
    FT_Bool         have_scale;
    FT_Pos          x, y;


    current.points   = gloader->base.outline.points +
                         num_base_points;
    current.n_points = gloader->base.outline.n_points -
                         (FT_UShort)num_base_points;

    have_scale = FT_BOOL( subglyph->flags & ( WE_HAVE_A_SCALE     |
                                              WE_HAVE_AN_XY_SCALE |
                                              WE_HAVE_A_2X2       ) );

    /* perform the transform required for this subglyph */
    if ( have_scale )
      FT_Outline_Transform( &current, &subglyph->transform );

    /* get offset */
    if ( !( subglyph->flags & ARGS_ARE_XY_VALUES ) )
    {
      FT_UInt     num_points = gloader->base.outline.n_points;
      FT_UInt     k = (FT_UInt)subglyph->arg1;
      FT_UInt     l = (FT_UInt)subglyph->arg2;
      FT_Vector*  p1;
      FT_Vector*  p2;


      /* match l-th point of the newly loaded component to the k-th point */
      /* of the previously loaded components.                             */

      /* change to the point numbers used by our outline */
      k += start_point;
      l += num_base_points;
      if ( k >= num_base_points ||
           l >= num_points      )
        return FT_THROW( Invalid_Composite );

      p1 = gloader->base.outline.points + k;
      p2 = gloader->base.outline.points + l;

      x = SUB_LONG( p1->x, p2->x );
      y = SUB_LONG( p1->y, p2->y );
    }
    else
    {
      x = subglyph->arg1;
      y = subglyph->arg2;

      if ( !x && !y )
        return FT_Err_Ok;

      /* Use a default value dependent on                                  */
      /* TT_CONFIG_OPTION_COMPONENT_OFFSET_SCALED.  This is useful for old */
      /* TT fonts which don't set the xxx_COMPONENT_OFFSET bit.            */

      if ( have_scale &&
#ifdef TT_CONFIG_OPTION_COMPONENT_OFFSET_SCALED
           !( subglyph->flags & UNSCALED_COMPONENT_OFFSET ) )
#else
            ( subglyph->flags & SCALED_COMPONENT_OFFSET ) )
#endif
      {

#if 0

        /********************************************************************
         *
         * This algorithm is what Apple documents.  But it doesn't work.
         */
        int  a = subglyph->transform.xx > 0 ?  subglyph->transform.xx
                                            : -subglyph->transform.xx;
        int  b = subglyph->transform.yx > 0 ?  subglyph->transform.yx
