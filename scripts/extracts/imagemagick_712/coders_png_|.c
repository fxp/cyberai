// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 60/94



     if (logging != MagickFalse)
       {
         (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "      image->colors=%d", (int) image->colors);

         if (image->colormap != NULL)
           {
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "       i     (red,green,blue,alpha)");

             for (i=0; i < (ssize_t) image->colors; i++)
             {
               if (i < 300 || i >= (ssize_t) image->colors - 10)
                 {
                   (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                       "       %d     (%d,%d,%d,%d)",
                        (int) i,
                        (int) image->colormap[i].red,
                        (int) image->colormap[i].green,
                        (int) image->colormap[i].blue,
                        (int) image->colormap[i].alpha);
                 }
             }
           }

           if (number_transparent < 257)
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "      number_transparent     = %d",
                   number_transparent);
           else

             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "      number_transparent     > 256");

           if (number_opaque < 257)
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "      number_opaque          = %d",
                   number_opaque);

           else
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "      number_opaque          > 256");

           if (number_semitransparent < 257)
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "      number_semitransparent = %d",
                   number_semitransparent);

           else
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "      number_semitransparent > 256");

           if (ping_have_non_bw == MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "      All pixels and the background are black or white");

           else if (ping_have_color == MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "      All pixels and the background are gray");

           else
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "      At least one pixel or the background is non-gray");

           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
               "    Exit BUILD_PALETTE:");
       }

   if (mng_info->write_png8 == MagickFalse)
      break;

   /* Make any reductions necessary for the PNG8 format */
    if (image_colors <= 256 &&
        image_colors != 0 && image->colormap != NULL &&
        number_semitransparent == 0 &&
        number_transparent <= 1)
      break;

    /* PNG8 can't have semitransparent colors so we threshold the
     * opacity to 0 or OpaqueOpacity, and PNG8 can only have one
     * transparent color so if more than one is transparent we merge
     * them into image->background_color.
     */
    if (number_semitransparent != 0 || number_transparent > 1)
      {
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Thresholding the alpha channel to binary");

        for (y=0; y < (ssize_t) image->rows; y++)
        {
          q=GetAuthenticPixels(image,0,y,image->columns,1,exception);

          if (q == (Quantum *) NULL)
            break;

          for (x=0; x < (ssize_t) image->columns; x++)
          {
              if (GetPixelAlpha(image,q) < OpaqueAlpha/2)
                {
                  SetPixelViaPixelInfo(image,&image->background_color,q);
                  SetPixelAlpha(image,TransparentAlpha,q);
                }
              else
                  SetPixelAlpha(image,OpaqueAlpha,q);
              q+=(ptrdiff_t) GetPixelChannels(image);
          }

          if (SyncAuthenticPixels(image,exception) == MagickFalse)
             break;

          if (image_colors != 0 && image_colors <= 256 &&
             image->colormap != NULL)
            for (i=0; i<image_colors; i++)
                image->colormap[i].alpha =
                    (image->colormap[i].alpha > (double) TransparentAlpha/2 ?
                    TransparentAlpha : OpaqueAlpha);
        }
      continue;
    }

    /* PNG8 can't have more than 256 colors so we quantize the pixels and
     * background color to the 4-4-4-1, 3-3-3-1 or 3-3-2-1 palette.  If the
     * image is mostly gray, the 4-4-4-1 palette is likely to end up with 256
     * colors or less.
     */
    if (tried_444 == MagickFalse && (image_colors == 0 || image_colors > 256))
      {
        if (logging != MagickFalse)
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
               "    Quantizing the background color to 4-4-4");

        tried_444 = MagickTrue;

        LBR04PacketRGB(image->background_color);