// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 54/94



    ping_have_blob,
    ping_have_cheap_transparency,
    ping_have_color,
    ping_have_non_bw,
    ping_have_PLTE,
    ping_have_bKGD,
    ping_have_eXIf,
    ping_have_iCCP,
    ping_have_pHYs,
    ping_have_sRGB,
    ping_have_tRNS,

    ping_need_colortype_warning,

    status,
    tried_332,
    tried_333,
    tried_444;

  MemoryInfo
    *volatile pixel_info;

  QuantumInfo
    *quantum_info;

  PNGErrorInfo
    error_info;

  ssize_t
    i,
    x;

  unsigned char
    *ping_pixels;

  volatile int
    image_colors,
    ping_bit_depth,
    ping_color_type,
    ping_interlace_method,
    ping_compression_method,
    ping_filter_method,
    ping_num_trans;

  volatile size_t
    image_depth,
    old_bit_depth;

  size_t
    quality,
    rowbytes,
    save_image_depth;

  int
    j,
    number_colors,
    number_opaque,
    number_semitransparent,
    number_transparent,
    ping_pHYs_unit_type;

  png_uint_32
    ping_pHYs_x_resolution,
    ping_pHYs_y_resolution;

  logging=LogMagickEvent(CoderEvent,GetMagickModule(),
    "  Enter WriteOnePNGImage()");

  image = CloneImage(IMimage,0,0,MagickFalse,exception);
  if (image == (Image *) NULL)
    return(MagickFalse);
  image_info=(ImageInfo *) CloneImageInfo(IMimage_info);

  /* Initialize some stuff */
  ping_bit_depth=0,
  ping_color_type=0,
  ping_interlace_method=0,
  ping_compression_method=0,
  ping_filter_method=0,
  ping_num_trans = 0;

  ping_background.red = 0;
  ping_background.green = 0;
  ping_background.blue = 0;
  ping_background.gray = 0;
  ping_background.index = 0;

  ping_trans_color.red=0;
  ping_trans_color.green=0;
  ping_trans_color.blue=0;
  ping_trans_color.gray=0;

  ping_pHYs_unit_type = 0;
  ping_pHYs_x_resolution = 0;
  ping_pHYs_y_resolution = 0;

  ping_have_blob=MagickFalse;
  ping_have_cheap_transparency=MagickFalse;
  ping_have_color=MagickTrue;
  ping_have_non_bw=MagickTrue;
  ping_have_PLTE=MagickFalse;
  ping_have_bKGD=MagickFalse;
  ping_have_eXIf=MagickTrue;
  ping_have_iCCP=MagickFalse;
  ping_have_pHYs=MagickFalse;
  ping_have_sRGB=MagickFalse;
  ping_have_tRNS=MagickFalse;

  ping_exclude_iCCP=mng_info->exclude_iCCP;
  ping_exclude_zCCP=mng_info->exclude_zCCP;

  ping_need_colortype_warning = MagickFalse;

  /* Recognize the ICC sRGB profile and convert it to the sRGB chunk,
   * i.e., eliminate the ICC profile and set image->rendering_intent.
   * Note that this will not involve any changes to the actual pixels
   * but merely passes information to applications that read the resulting
   * PNG image.
   *
   * To do: recognize other variants of the sRGB profile, using the CRC to
   * verify all recognized variants including the 7 already known.
   *
   * Work around libpng16+ rejecting some "known invalid sRGB profiles".
   *
   * Use something other than image->rendering_intent to record the fact
   * that the sRGB profile was found.
   *
   * Record the ICC version (currently v2 or v4) of the incoming sRGB ICC
   * profile.  Record the Blackpoint Compensation, if any.
   */
   if (mng_info->exclude_sRGB == MagickFalse && mng_info->preserve_iCCP == MagickFalse)
   {
      ResetImageProfileIterator(image);
      for (name=GetNextImageProfile(image); name != (char *) NULL; )
      {
        profile=GetImageProfile(image,name);

        if (profile != (StringInfo *) NULL)
          {
            if ((LocaleCompare(name,"ICC") == 0) ||
                (LocaleCompare(name,"ICM") == 0))

             {
                 int
                   icheck,
                   got_crc=0;


                 png_uint_32
                   length,
                   profile_crc=0;

                 unsigned char
                   *data;

                 length=(png_uint_32) GetStringInfoLength(profile);

                 for (icheck=0; sRGB_info[icheck].len > 0; icheck++)
                 {
                   if (length == sRGB_info[icheck].len)
                   {
                     if (got_crc == 0)
                     {
                       (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                         "    Got a %lu-byte ICC profile (potentially sRGB)",
                         (unsigned long) length);

                       data=GetStringInfoDatum(profile);
                       profile_crc=crc32(0,data,length);

                       (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                           "      with crc=%8x",(unsigned int) profile_crc);
                       got_crc++;
                     }