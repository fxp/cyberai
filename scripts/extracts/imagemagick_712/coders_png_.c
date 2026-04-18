// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 79/94



      else if (LocaleCompare(value,"png48") == 0)
        mng_info->write_png48 = MagickTrue;

      else if (LocaleCompare(value,"png64") == 0)
        mng_info->write_png64 = MagickTrue;

      else if ((LocaleCompare(value,"png00") == 0) ||
         LocaleCompare(image_info->magick,"PNG00") == 0)
        {
          /* Retrieve png:IHDR.bit-depth-orig and png:IHDR.color-type-orig. */
          value=GetImageProperty(image,"png:IHDR.bit-depth-orig",exception);

          if (value != (char *) NULL)
            {
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "  png00 inherited bit depth=%s",value);

              if (LocaleCompare(value,"1") == 0)
                mng_info->depth = 1;

              else if (LocaleCompare(value,"2") == 0)
                mng_info->depth = 2;

              else if (LocaleCompare(value,"4") == 0)
                mng_info->depth = 4;

              else if (LocaleCompare(value,"8") == 0)
                mng_info->depth = 8;

              else if (LocaleCompare(value,"16") == 0)
                mng_info->depth = 16;
            }

          value=GetImageProperty(image,"png:IHDR.color-type-orig",exception);

          if (value != (char *) NULL)
            {
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "  png00 inherited color type=%s",value);

              if (LocaleCompare(value,"0") == 0)
                mng_info->colortype=1;
              else if (LocaleCompare(value,"2") == 0)
                mng_info->colortype=3;
              else if (LocaleCompare(value,"3") == 0)
                mng_info->colortype=4;
              else if (LocaleCompare(value,"4") == 0)
                mng_info->colortype=5;
              else if (LocaleCompare(value,"6") == 0)
                mng_info->colortype=7;
            }
        }
    }

  if (mng_info->write_png8 != MagickFalse)
    {
      mng_info->colortype=4; /* 3 */
      mng_info->depth = 8;
      image->depth = 8;
      if (image->alpha_trait != UndefinedPixelTrait)
        (void) SetImageType(image,PaletteAlphaType,exception);
      else
        (void) SetImageType(image,PaletteType,exception);
    }

  if (mng_info->write_png24 != MagickFalse)
    {
      mng_info->colortype=3; /* 2 */
      mng_info->depth = 8;
      image->depth = 8;

      if (image->alpha_trait != UndefinedPixelTrait)
        (void) SetImageType(image,TrueColorAlphaType,exception);
      else
        (void) SetImageType(image,TrueColorType,exception);

      (void) SyncImage(image,exception);
    }

  if (mng_info->write_png32 != MagickFalse)
    {
      mng_info->colortype = /* 6 */  7;
      mng_info->depth = 8;
      image->depth = 8;
      if (image->alpha_trait != UndefinedPixelTrait)
        (void) SetImageType(image,TrueColorAlphaType,exception);
      else
        (void) SetImageType(image,TrueColorType,exception);
      (void) SyncImage(image,exception);
    }

  if (mng_info->write_png48 != MagickFalse)
    {
      mng_info->colortype = /* 2 */ 3;
      mng_info->depth = 16;
      image->depth = 16;
      if (image->alpha_trait != UndefinedPixelTrait)
        (void) SetImageType(image,TrueColorAlphaType,exception);
      else
        (void) SetImageType(image,TrueColorType,exception);
      (void) SyncImage(image,exception);
    }

  if (mng_info->write_png64 != MagickFalse)
    {
      mng_info->colortype = /* 6 */  7;
      mng_info->depth = 16;
      image->depth = 16;
      if (image->alpha_trait != UndefinedPixelTrait)
        (void) SetImageType(image,TrueColorAlphaType,exception);
      else
        (void) SetImageType(image,TrueColorType,exception);
      (void) SyncImage(image,exception);
    }

  value=GetImageOption(image_info,"png:bit-depth");

  if (value != (char *) NULL)
    {
      if (LocaleCompare(value,"1") == 0)
        mng_info->depth = 1;

      else if (LocaleCompare(value,"2") == 0)
        mng_info->depth = 2;

      else if (LocaleCompare(value,"4") == 0)
        mng_info->depth = 4;

      else if (LocaleCompare(value,"8") == 0)
        mng_info->depth = 8;

      else if (LocaleCompare(value,"16") == 0)
        mng_info->depth = 16;

      else
        (void) ThrowMagickException(exception,GetMagickModule(),CoderWarning,
          "ignoring invalid defined png:bit-depth","=%s",value);

      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "  png:bit-depth=%d was defined.\n",mng_info->depth);
    }

  value=GetImageOption(image_info,"png:color-type");

  if (value != (char *) NULL)
    {
      /* We must store colortype+1 because 0 is a valid colortype */
      if (LocaleCompare(value,"0") == 0)
        mng_info->colortype = 1;

      else if (LocaleCompare(value,"1") == 0)
        mng_info->colortype = 2;

      else if (LocaleCompare(value,"2") == 0)
        mng_info->colortype = 3;

      else if (LocaleCompare(value,"3") == 0)
        mng_info->colortype = 4;