// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 56/94



           for (y=0; y < (ssize_t) image->rows; y++)
           {
             r=GetAuthenticPixels(image,0,y,image->columns,1,exception);

             if (r == (Quantum *) NULL)
               break;

             for (x=0; x < (ssize_t) image->columns; x++)
             {
                LBR02PixelRGBA(r);
                r+=(ptrdiff_t) GetPixelChannels(image);
             }

             if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
           }

           if (image->storage_class == PseudoClass && image->colormap != NULL)
           {
             for (i=0; i < (ssize_t) image->colors; i++)
             {
               LBR02PacketRGBA(image->colormap[i]);
             }
           }
         }
       else
         {
           /* Scale to 1-bit */
           LBR01PacketRGBA(image->background_color);

           for (y=0; y < (ssize_t) image->rows; y++)
           {
             r=GetAuthenticPixels(image,0,y,image->columns,1,exception);

             if (r == (Quantum *) NULL)
               break;

             for (x=0; x < (ssize_t) image->columns; x++)
             {
                LBR01PixelRGBA(r);
                r+=(ptrdiff_t) GetPixelChannels(image);
             }

             if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
           }

           if (image->storage_class == PseudoClass && image->colormap != NULL)
           {
             for (i=0; i < (ssize_t) image->colors; i++)
             {
               LBR01PacketRGBA(image->colormap[i]);
             }
           }
         }
    }

  /* To do: set to next higher multiple of 8 */
  if (image->depth < 8)
     image->depth=8;

#if (MAGICKCORE_QUANTUM_DEPTH > 16)
  /* PNG does not handle depths greater than 16 so reduce it even
   * if lossy
   */
  if (image->depth > 8)
      image->depth=16;
#endif

#if (MAGICKCORE_QUANTUM_DEPTH > 8)
  if (image->depth > 8)
    {
      /* To do: fill low byte properly */
      image->depth=16;
    }

  if (image->depth == 16 && mng_info->depth != 16)
    if (mng_info->write_png8 != MagickFalse ||
        LosslessReduceDepthOK(image,exception) != MagickFalse)
      image->depth = 8;
#endif

  image_colors = (int) image->colors;
  number_opaque = (int) image->colors;
  number_transparent = 0;
  number_semitransparent = 0;
  if (IdentifyImageCoderGray(image,exception) != MagickFalse)
    ping_have_color=MagickFalse;

  if (mng_info->colortype &&
     (mng_info->colortype > 4 || (mng_info->depth >= 8 &&
     mng_info->colortype < 4 &&
     image->alpha_trait == UndefinedPixelTrait)))
  {
     /* Avoid the expensive BUILD_PALETTE operation if we're sure that we
      * are not going to need the result.
      */
     if (mng_info->colortype == 1 ||
        mng_info->colortype == 5)
       ping_have_color=MagickFalse;

     if (image->alpha_trait != UndefinedPixelTrait)
       {
         number_transparent = 2;
         number_semitransparent = 1;
       }
  }

  if (mng_info->colortype < 7)
  {
  /* BUILD_PALETTE
   *
   * Normally we run this just once, but in the case of writing PNG8
   * we reduce the transparency to binary and run again, then if there
   * are still too many colors we reduce to a simple 4-4-4-1, then 3-3-3-1
   * RGBA palette and run again, and then to a simple 3-3-2-1 RGBA
   * palette.  Then (To do) we take care of a final reduction that is only
   * needed if there are still 256 colors present and one of them has both
   * transparent and opaque instances.
   */

  tried_332 = MagickFalse;
  tried_333 = MagickFalse;
  tried_444 = MagickFalse;

  if (image->depth != GetImageDepth(image,exception))
    (void) SetImageDepth(image,image->depth,exception);
  for (j=0; j<6; j++)
  {
    /*
     * Sometimes we get DirectClass images that have 256 colors or fewer.
     * This code will build a colormap.
     *
     * Also, sometimes we get PseudoClass images with an out-of-date
     * colormap.  This code will replace the colormap with a new one.
     * Sometimes we get PseudoClass images that have more than 256 colors.
     * This code will delete the colormap and change the image to
     * DirectClass.
     *
     * If image->alpha_trait is MagickFalse, we ignore the alpha channel
     * even though it sometimes contains left-over non-opaque values.
     *
     * Also we gather some information (number of opaque, transparent,
     * and semitransparent pixels, and whether the image has any non-gray
     * pixels or only black-and-white pixels) that we might need later.
     *
     * Even if the user wants to force GrayAlpha or RGBA (colortype 4 or 6)
     * we need to check for bogus non-opaque values, at least.
     */

   int
     n;

   PixelInfo
     opaque[260],
     semitransparent[260],
     transparent[260];

   const Quantum
     *r;

   Quantum
     *q;

   if (logging != MagickFalse)
     (void) LogMagickEvent(CoderEvent,GetMagickModule(),
         "    Enter BUILD_PALETTE:");