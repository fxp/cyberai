  {
    FT_Error           error;
    CFF_Decoder_Zone*  zone;
    FT_Byte*           ip;
    FT_Byte*           limit;
    CFF_Builder*       builder = &decoder->builder;
    FT_Pos             x, y;
    FT_Fixed*          stack;
    FT_Int             charstring_type =
                         decoder->cff->top_font.font_dict.charstring_type;
    FT_UShort          num_designs =
                         decoder->cff->top_font.font_dict.num_designs;
    FT_UShort          num_axes =
                         decoder->cff->top_font.font_dict.num_axes;

    T2_Hints_Funcs  hinter;


    /* set default width */
    decoder->num_hints  = 0;
    decoder->read_width = 1;

    /* initialize the decoder */
    decoder->top  = decoder->stack;
    decoder->zone = decoder->zones;
    zone          = decoder->zones;
    stack         = decoder->top;

    hinter = (T2_Hints_Funcs)builder->hints_funcs;

    builder->path_begun = 0;

    if ( !charstring_base )
      return FT_Err_Ok;

    zone->base           = charstring_base;
    limit = zone->limit  = charstring_base + charstring_len;
    ip    = zone->cursor = zone->base;

    error = FT_Err_Ok;

    x = builder->pos_x;
    y = builder->pos_y;

    /* begin hints recording session, if any */
    if ( hinter )
      hinter->open( hinter->hints );

    /* now execute loop */
    while ( ip < limit )
    {
      CFF_Operator  op;
      FT_Byte       v;


      /*********************************************************************
       *
       * Decode operator or operand
       */
      v = *ip++;
      if ( v >= 32 || v == 28 )
      {
        FT_Int    shift = 16;
        FT_Int32  val;


        /* this is an operand, push it on the stack */

        /* if we use shifts, all computations are done with unsigned */
        /* values; the conversion to a signed value is the last step */
        if ( v == 28 )
        {
          if ( ip + 1 >= limit )
            goto Syntax_Error;
          val = (FT_Short)( ( (FT_UShort)ip[0] << 8 ) | ip[1] );
          ip += 2;
        }
        else if ( v < 247 )
          val = (FT_Int32)v - 139;
        else if ( v < 251 )
        {
          if ( ip >= limit )
            goto Syntax_Error;
          val = ( (FT_Int32)v - 247 ) * 256 + *ip++ + 108;
        }
        else if ( v < 255 )
        {
          if ( ip >= limit )
            goto Syntax_Error;
          val = -( (FT_Int32)v - 251 ) * 256 - *ip++ - 108;
        }
        else
        {
          if ( ip + 3 >= limit )
            goto Syntax_Error;
          val = (FT_Int32)( ( (FT_UInt32)ip[0] << 24 ) |
                            ( (FT_UInt32)ip[1] << 16 ) |
                            ( (FT_UInt32)ip[2] <<  8 ) |
                              (FT_UInt32)ip[3]         );
          ip += 4;
          if ( charstring_type == 2 )
            shift = 0;
        }
        if ( decoder->top - stack >= CFF_MAX_OPERANDS )
          goto Stack_Overflow;

        val             = (FT_Int32)( (FT_UInt32)val << shift );
        *decoder->top++ = val;

#ifdef FT_DEBUG_LEVEL_TRACE
        if ( !( val & 0xFFFFL ) )
          FT_TRACE4(( " %hd", (FT_Short)( (FT_UInt32)val >> 16 ) ));
        else
          FT_TRACE4(( " %.5f", val / 65536.0 ));
#endif

      }
      else
      {
        /* The specification says that normally arguments are to be taken */
        /* from the bottom of the stack.  However, this seems not to be   */
        /* correct, at least for Acroread 7.0.8 on GNU/Linux: It pops the */
        /* arguments similar to a PS interpreter.                         */

        FT_Fixed*  args     = decoder->top;
        FT_Int     num_args = (FT_Int)( args - decoder->stack );
        FT_Int     req_args;


        /* find operator */
        op = cff_op_unknown;

        switch ( v )
        {
        case 1:
          op = cff_op_hstem;
          break;
        case 3:
          op = cff_op_vstem;
          break;
        case 4:
          op = cff_op_vmoveto;
          break;
        case 5:
          op = cff_op_rlineto;
          break;
        case 6:
          op = cff_op_hlineto;
          break;
        case 7:
          op = cff_op_vlineto;
          break;
        case 8:
          op = cff_op_rrcurveto;
          break;
        case 9:
          op = cff_op_closepath;
          break;
        case 10:
          op = cff_op_callsubr;
