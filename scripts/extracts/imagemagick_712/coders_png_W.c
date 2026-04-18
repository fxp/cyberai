// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 55/94



                     if (profile_crc == sRGB_info[icheck].crc)
                     {
                        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "      It is sRGB with rendering intent = %s",
                        Magick_RenderingIntentString_from_PNG_RenderingIntent(
                             sRGB_info[icheck].intent));
                        if (image->rendering_intent==UndefinedIntent)
                        {
                          image->rendering_intent=
                          Magick_RenderingIntent_from_PNG_RenderingIntent(
                             sRGB_info[icheck].intent);
                        }
                        ping_exclude_iCCP = MagickTrue;
                        ping_exclude_zCCP = MagickTrue;
                        ping_have_sRGB = MagickTrue;
                        break;
                     }
                   }
                 }
                 if (sRGB_info[icheck].len == 0)
                    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                        "    Got %lu-byte ICC profile not recognized as sRGB",
                        (unsigned long) length);
              }
          }
        name=GetNextImageProfile(image);
      }
  }

  number_opaque = 0;
  number_semitransparent = 0;
  number_transparent = 0;

  if (logging != MagickFalse)
    {
      if (image->storage_class == UndefinedClass)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    image->storage_class=UndefinedClass");
      if (image->storage_class == DirectClass)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    image->storage_class=DirectClass");
      if (image->storage_class == PseudoClass)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    image->storage_class=PseudoClass");
      (void) LogMagickEvent(CoderEvent,GetMagickModule(), image->taint ?
          "    image->taint=MagickTrue":
          "    image->taint=MagickFalse");
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    image->gamma=%g", image->gamma);
    }

  if (image->storage_class == PseudoClass &&
     ((mng_info->write_png8 != MagickFalse) ||
      (mng_info->write_png24 != MagickFalse) ||
      (mng_info->write_png32 != MagickFalse) ||
      (mng_info->write_png48 != MagickFalse) ||
      (mng_info->write_png64 != MagickFalse) ||
      (mng_info->colortype == 3) || // PNG_COLOR_TYPE_RGB
      (mng_info->colortype == 7))) // PNG_COLOR_TYPE_RGB_ALPHA
    {
      (void) SyncImage(image,exception);
      image->storage_class = DirectClass;
    }

  if (mng_info->preserve_colormap == MagickFalse)
    {
      if ((image->storage_class != PseudoClass) &&
          (image->colormap != (PixelInfo *) NULL))
        {
          /* Free the bogus colormap; it can cause trouble later */
           if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "    Freeing bogus colormap");
           image->colormap=(PixelInfo *) RelinquishMagickMemory(
             image->colormap);
        }
    }

  if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
    (void) TransformImageColorspace(image,sRGBColorspace,exception);

  /*
    Sometimes we get PseudoClass images whose RGB values don't match
    the colors in the colormap.  This code syncs the RGB values.
  */
  image->depth=GetImageQuantumDepth(image,MagickFalse);
  if (image->depth <= 8 && image->taint && image->storage_class == PseudoClass)
     (void) SyncImage(image,exception);

#if (MAGICKCORE_QUANTUM_DEPTH == 8)
  if (image->depth > 8)
    {
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    Reducing PNG bit depth to 8 since this is a Q8 build.");

      image->depth=8;
    }
#endif

  /* Respect the -depth option */
  if (image->depth < 4)
    {
       Quantum
         *r;

       if (image->depth > 2)
         {
           /* Scale to 4-bit */
           LBR04PacketRGBA(image->background_color);

           for (y=0; y < (ssize_t) image->rows; y++)
           {
             r=GetAuthenticPixels(image,0,y,image->columns,1,exception);

             if (r == (Quantum *) NULL)
               break;

             for (x=0; x < (ssize_t) image->columns; x++)
             {
                LBR04PixelRGBA(r);
                r+=(ptrdiff_t) GetPixelChannels(image);
             }

             if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
           }

           if (image->storage_class == PseudoClass && image->colormap != NULL)
           {
             for (i=0; i < (ssize_t) image->colors; i++)
             {
               LBR04PacketRGBA(image->colormap[i]);
             }
           }
         }
       else if (image->depth > 1)
         {
           /* Scale to 2-bit */
           LBR02PacketRGBA(image->background_color);