// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 53/94



  assert(timestamp != (const char *) NULL);
  LogMagickEvent(CoderEvent,GetMagickModule(),
      "  Writing tIME chunk: timestamp property is %30s\n",timestamp);
  ret=MagickSscanf(timestamp,"%d-%d-%dT%d:%d:%d",&year,&month,&day,&hour,
      &minute, &second);
  addhours=0;
  addminutes=0;
  ret=MagickSscanf(timestamp,"%d-%d-%dT%d:%d:%d%d:%d",&year,&month,&day,&hour,
      &minute, &second, &addhours, &addminutes);
    LogMagickEvent(CoderEvent,GetMagickModule(),
      "   Date format specified for png:tIME=%s" ,timestamp);
    LogMagickEvent(CoderEvent,GetMagickModule(),
      "      ret=%d,y=%d, m=%d, d=%d, h=%d, m=%d, s=%d, ah=%d, as=%d",
      ret,year,month,day,hour,minute,second,addhours,addminutes);
  if (ret < 6)
  {
    LogMagickEvent(CoderEvent,GetMagickModule(),
      "      Invalid date, ret=%d",ret);
    (void) ThrowMagickException(exception,GetMagickModule(),CoderError,
      "Invalid date format specified for png:tIME","`%s' (ret=%d)",
      image->filename,ret);
    return;
  }
  if (addhours < 0)
  {
    addhours+=24;
    addminutes=-addminutes;
    day--;
  }
  hour+=addhours;
  minute+=addminutes;
  if (day == 0)
  {
    month--;
    day=31;
    if(month == 2)
      day=28;
    else
    {
      if(month == 4 || month == 6 || month == 9 || month == 11)
        day=30;
      else
        day=31;
    }
  }
  if (month == 0)
  {
    month++;
    year--;
  }
  if (minute > 59)
  {
     hour++;
     minute-=60;
  }
  if (hour > 23)
  {
     day ++;
     hour -=24;
  }
  if (hour < 0)
  {
     day --;
     hour +=24;
  }
  /* To do: fix this for leap years */
  if (day > 31 || (month == 2 && day > 28) || ((month == 4 || month == 6 ||
      month == 9 || month == 11) && day > 30))
  {
     month++;
     day = 1;
  }
  if (month > 12)
  {
     year++;
     month=1;
  }

  ptime.year = (png_uint_16) year;
  ptime.month = (png_byte) month;
  ptime.day = (png_byte) day;
  ptime.hour = (png_byte) hour;
  ptime.minute = (png_byte) minute;
  ptime.second = (png_byte) second;
  png_convert_from_time_t(&ptime,GetMagickTime());
  LogMagickEvent(CoderEvent,GetMagickModule(),
      "      png_set_tIME: y=%d, m=%d, d=%d, h=%d, m=%d, s=%d, ah=%d, am=%d",
      ptime.year, ptime.month, ptime.day, ptime.hour, ptime.minute,
      ptime.second, addhours, addminutes);
  png_set_tIME(ping,info,&ptime);
}
#endif

static void Magick_png_set_text(png_struct *ping,png_info *ping_info,
  MngWriteInfo *mng_info, const ImageInfo *image_info,const char* key,
  const char *value)
{
#if PNG_LIBPNG_VER >= 10600
  const char
    *c;

  MagickBooleanType
    write_itxt=MagickFalse;
#endif

  int
    compresion_none=PNG_TEXT_COMPRESSION_NONE,
    compresion_zTXt=PNG_TEXT_COMPRESSION_zTXt;

  png_textp
    text;

  size_t
    length;

#if PNG_LIBPNG_VER >= 10600
  /*
    Check if the string contains non-Latin1 characters.
  */
  c=value;
  while(*c != '\0')
  {
      if (((const unsigned char) *c) > 127) {
        write_itxt=MagickTrue;
        compresion_none=PNG_ITXT_COMPRESSION_NONE;
        compresion_zTXt=PNG_ITXT_COMPRESSION_zTXt;
        break;
      }
      c++;
  }
#endif
#if PNG_LIBPNG_VER >= 10400
  text=(png_textp) png_malloc(ping,(png_alloc_size_t) sizeof(png_text));
#else
  text=(png_textp) png_malloc(ping,(png_size_t) sizeof(png_text));
#endif
  if (text == (png_textp) NULL)
    return;
  (void) memset(text,0,sizeof(png_text));
  text[0].key=(char *) key;
  text[0].text=(char *) value;
  length=strlen(value);
#if PNG_LIBPNG_VER >= 10600
  if (write_itxt != MagickFalse)
    text[0].itxt_length=length;
  else
#endif
    text[0].text_length=length;
  if (mng_info->exclude_tEXt != MagickFalse)
    text[0].compression=compresion_zTXt;
  else
    if (mng_info->exclude_zTXt != MagickFalse)
      text[0].compression=compresion_none;
    else
      text[0].compression=
        image_info->compression == NoCompression ||
        (image_info->compression == UndefinedCompression &&
        (length < 128)) ?  compresion_none : compresion_zTXt;
  png_set_text(ping,ping_info,text,1);
  png_free(ping,text);
}

/* Write one PNG image */
static MagickBooleanType WriteOnePNGImage(MngWriteInfo *mng_info,
  const ImageInfo *IMimage_info,Image *IMimage,ExceptionInfo *exception)
{
  Image
    *image;

  ImageInfo
    *image_info;

  char
    *name,
    s[2];

  const char
    *property,
    *value;

  const StringInfo
    *profile;

  int
    num_passes,
    pass,
    ping_wrote_caNv;

  png_byte
     ping_trans_alpha[257];

  png_color
     palette[257];

  png_color_16
    ping_background,
    ping_trans_color;

  png_info
    *ping_info;

  png_struct
    *ping;

  png_uint_32
    ping_height,
    ping_width;

  ssize_t
    y;

  MagickBooleanType
    image_matte,
    logging = MagickFalse,
    matte,

    ping_exclude_iCCP,
    ping_exclude_zCCP,