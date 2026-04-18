// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 52/94



static void Magick_png_write_raw_profile(const ImageInfo *image_info,
  png_struct *ping,png_info *ping_info,unsigned char *profile_type,
  unsigned char *profile_description,unsigned char *profile_data,
  png_uint_32 length,ExceptionInfo *exception)
{
   png_charp
     dp;

   png_textp
     text;

   png_uint_32
     allocated_length,
     description_length;

   ssize_t
     i;

   unsigned char
     hex[16] =
       { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' },
     *sp;

   if ((length > 10) && (*profile_type != '\0'))
     {
       if (LocaleNCompare((char *) profile_type+1, "ng-chunk-",9) == 0)
          return;
     }
   if (image_info->verbose != MagickFalse)
     {
       (void) printf("writing raw profile: type=%s, length=%.20g\n",
         (char *) profile_type, (double) length);
     }
   description_length=(png_uint_32) strlen((const char *) profile_description);
   allocated_length=(png_uint_32) (2*length+(length >> 5)+description_length+
     20);
   if ((allocated_length < length) || (length >= (PNG_UINT_31_MAX / 2)))
     {
       (void) ThrowMagickException(exception,GetMagickModule(),CoderError,
         "maximum profile length exceeded","`%s'",image_info->filename);
       return;
     }
#if PNG_LIBPNG_VER >= 10400
   text=(png_textp) png_malloc(ping,(png_alloc_size_t) sizeof(png_text));
#else
   text=(png_textp) png_malloc(ping,(png_size_t) sizeof(png_text));
#endif
#if PNG_LIBPNG_VER >= 10400
   text[0].text=(png_charp) png_malloc(ping,(png_alloc_size_t)
     allocated_length);
   text[0].key=(png_charp) png_malloc(ping,(png_alloc_size_t) 80);
#else
   text[0].text=(png_charp) png_malloc(ping,(png_size_t) allocated_length);
   text[0].key=(png_charp) png_malloc(ping,(png_size_t) 80);
#endif
   text[0].key[0]='\0';
   (void) ConcatenateMagickString(text[0].key,"Raw profile type ",
     MagickPathExtent);
   (void) ConcatenateMagickString(text[0].key,(const char *) profile_type,62);
   sp=profile_data;
   dp=text[0].text;
   *dp++='\n';
   (void) CopyMagickString(dp,(const char *) profile_description,
     allocated_length);
   dp+=description_length;
   *dp++='\n';
   (void) FormatLocaleString(dp,allocated_length-
     (png_size_t) (dp-text[0].text),"%8lu",(unsigned long) length);
   dp+=8;

   for (i=0; i < (ssize_t) length; i++)
   {
     if (i%36 == 0)
       *dp++='\n';
     *(dp++)=(char) hex[((*sp >> 4) & 0x0f)];
     *(dp++)=(char) hex[((*sp++ ) & 0x0f)];
   }

   *dp++='\n';
   *dp='\0';
   text[0].text_length=(png_size_t) (dp-text[0].text);
   text[0].compression=image_info->compression == NoCompression ||
     (image_info->compression == UndefinedCompression &&
     text[0].text_length < 128) ? -1 : 0;

   if (text[0].text_length <= allocated_length)
     png_set_text(ping,ping_info,text,1);

   png_free(ping,text[0].text);
   png_free(ping,text[0].key);
   png_free(ping,text);
}

static inline MagickBooleanType IsColorEqual(const Image *image,
  const Quantum *p, const PixelInfo *q)
{
  MagickRealType
    blue,
    green,
    red;

  red=(MagickRealType) GetPixelRed(image,p);
  green=(MagickRealType) GetPixelGreen(image,p);
  blue=(MagickRealType) GetPixelBlue(image,p);
  if ((AbsolutePixelValue(red-q->red) < MagickEpsilon) &&
      (AbsolutePixelValue(green-q->green) < MagickEpsilon) &&
      (AbsolutePixelValue(blue-q->blue) < MagickEpsilon))
    return(MagickTrue);
  return(MagickFalse);
}

#if defined(PNG_tIME_SUPPORTED)
static void write_tIME_chunk(Image *image,png_struct *ping,png_info *info,
  const char *timestamp,ExceptionInfo *exception)
{
  int
    ret;

  int
    day,
    hour,
    minute,
    month,
    second,
    year;

  int
    addhours=0,
    addminutes=0;

  png_time
    ptime;