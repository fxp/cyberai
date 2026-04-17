  {
    FT_Error  error = FT_Err_Ok;
    FT_Byte*  sfnt  = *sfnt_bytes;

    /* current position in stream */
    const FT_ULong  pos = FT_STREAM_POS();

    FT_UInt  num_substreams = 7;

    FT_UShort  option_flags;
    FT_UShort  num_glyphs;
    FT_UShort  index_format;
    FT_ULong   expected_loca_length;
    FT_UInt    offset;
    FT_UInt    i;
    FT_ULong   points_size;
    FT_ULong   glyph_buf_size;
    FT_ULong   bbox_bitmap_offset;
    FT_ULong   bbox_bitmap_length;
    FT_ULong   overlap_bitmap_offset = 0;
    FT_ULong   overlap_bitmap_length = 0;

    const FT_ULong  glyf_start  = *out_offset;
    FT_ULong        dest_offset = *out_offset;

    WOFF2_Substream  substreams = NULL;

    FT_ULong*    loca_values  = NULL;
    FT_UShort*   n_points_arr = NULL;
    FT_Byte*     glyph_buf    = NULL;
    WOFF2_Point  points       = NULL;


    if ( FT_QNEW_ARRAY( substreams, num_substreams ) )
      goto Fail;

    if ( FT_STREAM_SKIP( 2 ) )
      goto Fail;
    if ( FT_READ_USHORT( option_flags ) )
      goto Fail;
    if ( FT_READ_USHORT( num_glyphs ) )
      goto Fail;
    if ( FT_READ_USHORT( index_format ) )
      goto Fail;

    FT_TRACE4(( "option_flags = %u; num_glyphs = %u; index_format = %u\n",
                option_flags, num_glyphs, index_format ));

    info->num_glyphs = num_glyphs;

    /* Calculate expected length of loca and compare.          */
    /* See https://www.w3.org/TR/WOFF2/#conform-mustRejectLoca */
    /* index_format = 0 => Short version `loca'.               */
    /* index_format = 1 => Long version `loca'.                */
    expected_loca_length = ( index_format ? 4 : 2 ) *
                             ( (FT_ULong)num_glyphs + 1 );
    if ( info->loca_table->dst_length != expected_loca_length )
      goto Fail;

    offset = 2 + 2 + 2 + 2 + ( num_substreams * 4 );
    if ( offset > info->glyf_table->TransformLength )
      goto Fail;

    for ( i = 0; i < num_substreams; ++i )
    {
      FT_ULong  substream_size;


      if ( FT_READ_ULONG( substream_size ) )
        goto Fail;
      if ( substream_size > info->glyf_table->TransformLength - offset )
        goto Fail;

      substreams[i].start  = pos + offset;
      substreams[i].offset = pos + offset;
      substreams[i].size   = substream_size;

      FT_TRACE5(( "  Substream %d: offset = %lu; size = %lu;\n",
                  i, substreams[i].offset, substreams[i].size ));
      offset += substream_size;
    }

    if ( option_flags & HAVE_OVERLAP_SIMPLE_BITMAP )
    {
      /* Size of overlapBitmap = floor((numGlyphs + 7) / 8) */
      overlap_bitmap_length = ( num_glyphs + 7U ) >> 3;
      if ( overlap_bitmap_length > info->glyf_table->TransformLength - offset )
        goto Fail;

      overlap_bitmap_offset = pos + offset;

      FT_TRACE5(( "  Overlap bitmap: offset = %lu; size = %lu;\n",
                  overlap_bitmap_offset, overlap_bitmap_length ));
      offset += overlap_bitmap_length;
    }

    if ( FT_QNEW_ARRAY( loca_values, num_glyphs + 1 ) )
      goto Fail;

    points_size        = 0;
    bbox_bitmap_offset = substreams[BBOX_STREAM].offset;

    /* Size of bboxBitmap = 4 * floor((numGlyphs + 31) / 32) */
    bbox_bitmap_length              = ( ( num_glyphs + 31U ) >> 5 ) << 2;
    /* bboxStreamSize is the combined size of bboxBitmap and bboxStream. */
    substreams[BBOX_STREAM].offset += bbox_bitmap_length;

    glyph_buf_size = WOFF2_DEFAULT_GLYPH_BUF;
    if ( FT_QALLOC( glyph_buf, glyph_buf_size ) )
      goto Fail;

    if ( FT_QNEW_ARRAY( info->x_mins, num_glyphs ) )
      goto Fail;

    for ( i = 0; i < num_glyphs; ++i )
    {
      FT_ULong   glyph_size = 0;
      FT_UShort  n_contours = 0;
      FT_Bool    have_bbox  = FALSE;
      FT_Byte    bbox_bitmap;
      FT_ULong   bbox_offset;
      FT_UShort  x_min      = 0;


      /* Set `have_bbox'. */
      bbox_offset = bbox_bitmap_offset + ( i >> 3 );
      if ( FT_STREAM_SEEK( bbox_offset ) ||
           FT_READ_BYTE( bbox_bitmap )   )
        goto Fail;
      if ( bbox_bitmap & ( 0x80 >> ( i & 7 ) ) )
        have_bbox = TRUE;

      /* Read value from `nContourStream'. */
      if ( FT_STREAM_SEEK( substreams[N_CONTOUR_STREAM].offset ) ||
           FT_READ_USHORT( n_contours )                          )
        goto Fail;
      substreams[N_CONTOUR_STREAM].offset += 2;

      if ( n_contours == 0xffff )
      {
        /* composite glyph */
        FT_Bool    have_instructions = FALSE;
        FT_UShort  instruction_size  = 0;
        FT_ULong   composite_size    = 0;
        FT_ULong   size_needed;
        FT_Byte*   pointer           = NULL;


        /* Composite glyphs must have explicit bbox. */
        if ( !have_bbox )
          goto Fail;

        if ( compositeGlyph_size( stream,
                                  substreams[COMPOSITE_STREAM].offset,
                                  &composite_size,
                                  &have_instructions) )
          goto Fail;

        if ( have_instructions )
        {
          if ( FT_STREAM_SEEK( substreams[GLYPH_STREAM].offset ) ||
               READ_255USHORT( instruction_size )                )
            goto Fail;
