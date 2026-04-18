// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 72/94



      if (image->taint == MagickFalse)
        {
          timestamp=GetImageOption(image_info,"png:tIME");

          if (timestamp == (const char *) NULL)
            timestamp=GetImageProperty(image,"png:tIME",exception);
        }

      else
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "  Reset tIME in tainted image");

          timestamp=GetImageProperty(image,"date:modify",exception);
        }

      if (timestamp != (const char *) NULL)
          write_tIME_chunk(image,ping,ping_info,timestamp,exception);
    }
#endif

  if (mng_info->need_blob != MagickFalse)
  {
    if (OpenBlob(image_info,image,WriteBinaryBlobMode,exception) ==
       MagickFalse)
       png_error(ping,"WriteBlob Failed");

     ping_have_blob=MagickTrue;
  }

  png_write_info_before_PLTE(ping, ping_info);

  if (ping_have_tRNS != MagickFalse && ping_color_type < 4)
    {
      if (logging != MagickFalse)
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Calling png_set_tRNS with num_trans=%d",ping_num_trans);
        }

      if (ping_color_type == 3)
         (void) png_set_tRNS(ping, ping_info,
                ping_trans_alpha,
                ping_num_trans,
                NULL);

      else
        {
           (void) png_set_tRNS(ping, ping_info,
                  NULL,
                  0,
                  &ping_trans_color);

           if (logging != MagickFalse)
             {
               (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "     tRNS color   =(%d,%d,%d)",
                       (int) ping_trans_color.red,
                       (int) ping_trans_color.green,
                       (int) ping_trans_color.blue);
             }
         }
    }

  /*
    Generate text chunks.
  */
  if (mng_info->exclude_tEXt == MagickFalse || mng_info->exclude_zTXt == MagickFalse)
  {
    ResetImagePropertyIterator(image);
    while ((property=GetNextImageProperty(image)) != (const char *) NULL)
    {
      /* Don't write any "png:" or "jpeg:" properties; those are just for
       * "identify" or for passing through to another JPEG
       */
      if ((LocaleNCompare(property,"png:",4) == 0 ||
           LocaleNCompare(property,"jpeg:",5) == 0))
        continue;
      /* Suppress density and units if we wrote a pHYs chunk */
      if ((mng_info->exclude_pHYs == MagickFalse) && (
          ((LocaleCompare(property,"exif:ResolutionUnit") == 0) ||
           (LocaleCompare(property,"exif:XResolution") == 0) ||
           (LocaleCompare(property,"exif:YResolution") == 0) ||
           (LocaleCompare(property,"tiff:ResolutionUnit") == 0) ||
           (LocaleCompare(property,"tiff:XResolution") == 0) ||
           (LocaleCompare(property,"tiff:YResolution") == 0) ||
           (LocaleCompare(property,"density") == 0) ||
           (LocaleCompare(property,"units") == 0))))
        continue;
      /* Suppress the IM-generated date:create and date:modify */
      if ((mng_info->exclude_date != MagickFalse) &&
          (LocaleNCompare(property, "date:",5) == 0))
        continue;
      value=GetImageProperty(image,property,exception);
      if (value == (const char *) NULL)
        continue;
      Magick_png_set_text(ping,ping_info,mng_info,image_info,property,value);
    }
  }

  /* write eXIf profile */
  if (ping_have_eXIf != MagickFalse && mng_info->exclude_eXIf == MagickFalse)
    {
      ResetImageProfileIterator(image);

      for (name=GetNextImageProfile(image); name != (char *) NULL; )
      {
        if (LocaleCompare(name,"exif") == 0)
          {
            profile=GetImageProfile(image,name);

            if (profile != (StringInfo *) NULL)
              {
                png_uint_32
                  length;

                unsigned char
                  chunk[4],
                  *data;

                StringInfo
                  *ping_profile;

                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "  Have eXIf profile");

                ping_profile=CloneStringInfo(profile);
                data=GetStringInfoDatum(ping_profile),
                length=(png_uint_32) GetStringInfoLength(ping_profile);

                PNGType(chunk,mng_eXIf);
                if (length < 7)
                  {
                    ping_profile=DestroyStringInfo(ping_profile);
                    break;  /* otherwise crashes */
                  }

                if (*data == 'E' && *(data+1) == 'x' && *(data+2) == 'i' &&
                    *(data+3) == 'f' && *(data+4) == '\0' && *(data+5) == '\0')
                  {
                    /* skip the "Exif\0\0" JFIF Exif Header ID */
                    length -= 6;
                    data += 6;
                  }