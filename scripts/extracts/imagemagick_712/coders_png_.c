// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 82/94



    if (IsOptionMember("none",value) != MagickFalse)
      {
        mng_info->exclude_bKGD=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_caNv=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_cHRM=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_date=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_eXIf=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_EXIF=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_gAMA=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_iCCP=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_oFFs=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_pHYs=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_sRGB=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_tEXt=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_tIME=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_tRNS=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_zCCP=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
        mng_info->exclude_zTXt=excluding != MagickFalse ? MagickFalse :
          MagickTrue;
      }

    if (IsOptionMember("bkgd",value) != MagickFalse)
      mng_info->exclude_bKGD=excluding;

    if (IsOptionMember("caNv",value) != MagickFalse)
      mng_info->exclude_caNv=excluding;

    if (IsOptionMember("chrm",value) != MagickFalse)
      mng_info->exclude_cHRM=excluding;

    if (IsOptionMember("date",value) != MagickFalse)
      mng_info->exclude_date=excluding;

    if (IsOptionMember("exif",value) != MagickFalse)
      {
        mng_info->exclude_EXIF=excluding;
        mng_info->exclude_eXIf=excluding;
      }

    if (IsOptionMember("gama",value) != MagickFalse)
      mng_info->exclude_gAMA=excluding;

    if (IsOptionMember("iccp",value) != MagickFalse)
      mng_info->exclude_iCCP=excluding;

    if (IsOptionMember("offs",value) != MagickFalse)
      mng_info->exclude_oFFs=excluding;

    if (IsOptionMember("phys",value) != MagickFalse)
      mng_info->exclude_pHYs=excluding;

    if (IsOptionMember("srgb",value) != MagickFalse)
      mng_info->exclude_sRGB=excluding;

    if (IsOptionMember("text",value) != MagickFalse)
      mng_info->exclude_tEXt=excluding;

    if (IsOptionMember("time",value) != MagickFalse)
      mng_info->exclude_tIME=excluding;

    if (IsOptionMember("trns",value) != MagickFalse)
      mng_info->exclude_tRNS=excluding;

    if (IsOptionMember("zccp",value) != MagickFalse)
      mng_info->exclude_zCCP=excluding;

    if (IsOptionMember("ztxt",value) != MagickFalse)
      mng_info->exclude_zTXt=excluding;
  }