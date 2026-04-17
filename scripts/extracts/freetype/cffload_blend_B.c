      /* Push blended result as Type 2 5-byte fixed-point number.  This */
      /* will not conflict with actual DICTs because 255 is a reserved  */
      /* opcode in both CFF and CFF2 DICTs.  See `cff_parse_num' for    */
      /* decode of this, which rounds to an integer.                    */
      *subFont->blend_top++ = 255;
      *subFont->blend_top++ = (FT_Byte)( (FT_UInt32)sum >> 24 );
      *subFont->blend_top++ = (FT_Byte)( (FT_UInt32)sum >> 16 );
      *subFont->blend_top++ = (FT_Byte)( (FT_UInt32)sum >>  8 );
      *subFont->blend_top++ = (FT_Byte)( (FT_UInt32)sum );
    }

    /* leave only numBlends results on parser stack */
    parser->top = &parser->stack[base + numBlends];

  Exit:
    return error;
  }


  /* Compute a blend vector from variation store index and normalized  */
  /* vector based on pseudo-code in OpenType Font Variations Overview. */
  /*                                                                   */
  /* Note: lenNDV == 0 produces a default blend vector, (1,0,0,...).   */
  FT_LOCAL_DEF( FT_Error )
  cff_blend_build_vector( CFF_Blend  blend,
                          FT_UInt    vsindex,
                          FT_UInt    lenNDV,
                          FT_Fixed*  NDV )
  {
    FT_Error   error  = FT_Err_Ok;            /* for FT_REALLOC */
    FT_Memory  memory = blend->font->memory;  /* for FT_REALLOC */

    FT_UInt       len;
    CFF_VStore    vs;
    CFF_VarData*  varData;
    FT_UInt       master;


    /* protect against malformed fonts */
    if ( !( lenNDV == 0 || NDV ) )
    {
      FT_TRACE4(( " cff_blend_build_vector:"
                  " Malformed Normalize Design Vector data\n" ));
      error = FT_THROW( Invalid_File_Format );
      goto Exit;
    }

    blend->builtBV = FALSE;

    vs = &blend->font->vstore;

    /* VStore and fvar must be consistent */
    if ( lenNDV != 0 && lenNDV != vs->axisCount )
    {
      FT_TRACE4(( " cff_blend_build_vector: Axis count mismatch\n" ));
      error = FT_THROW( Invalid_File_Format );
      goto Exit;
    }

    if ( vsindex >= vs->dataCount )
    {
      FT_TRACE4(( " cff_blend_build_vector: vsindex out of range\n" ));
      error = FT_THROW( Invalid_File_Format );
      goto Exit;
    }

    /* select the item variation data structure */
    varData = &vs->varData[vsindex];

    /* prepare buffer for the blend vector */
    len = varData->regionIdxCount + 1;    /* add 1 for default component */
    if ( FT_QRENEW_ARRAY( blend->BV, blend->lenBV, len ) )
      goto Exit;

    blend->lenBV = len;

    /* outer loop steps through master designs to be blended */
    for ( master = 0; master < len; master++ )
    {
      FT_UInt         j;
      FT_UInt         idx;
      CFF_VarRegion*  varRegion;


      /* default factor is always one */
      if ( master == 0 )
      {
        blend->BV[master] = FT_FIXED_ONE;
        FT_TRACE4(( "   build blend vector len %d\n", len ));
        FT_TRACE4(( "   [ %f ", blend->BV[master] / 65536.0 ));
        continue;
      }

      /* VStore array does not include default master, so subtract one */
      idx       = varData->regionIndices[master - 1];
      varRegion = &vs->varRegionList[idx];

      if ( idx >= vs->regionCount )
      {
        FT_TRACE4(( " cff_blend_build_vector:"
                    " region index out of range\n" ));
        error = FT_THROW( Invalid_File_Format );
        goto Exit;
      }

      /* Note: `lenNDV' could be zero.                              */
      /*       In that case, build default blend vector (1,0,0...). */
      if ( !lenNDV )
      {
        blend->BV[master] = 0;
        continue;
      }

      /* In the normal case, initialize each component to 1 */
      /* before inner loop.                                 */
      blend->BV[master] = FT_FIXED_ONE; /* default */

      /* inner loop steps through axes in this region */
      for ( j = 0; j < lenNDV; j++ )
      {
        CFF_AxisCoords*  axis = &varRegion->axisList[j];


        /* compute the scalar contribution of this axis */
        /* with peak of 0 used for invalid axes         */
        if ( axis->peakCoord == NDV[j] ||
             axis->peakCoord == 0      )
          continue;

        /* ignore this region if coords are out of range */
        else if ( NDV[j] <= axis->startCoord ||
                  NDV[j] >= axis->endCoord   )
