// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 61/94



        if (logging != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "    Quantizing the pixel colors to 4-4-4");

        if (image->colormap == NULL)
        {
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            q=GetAuthenticPixels(image,0,y,image->columns,1,exception);

            if (q == (Quantum *) NULL)
              break;

            for (x=0; x < (ssize_t) image->columns; x++)
            {
              if (GetPixelAlpha(image,q) == OpaqueAlpha)
                  LBR04PixelRGB(q);
              q+=(ptrdiff_t) GetPixelChannels(image);
            }

            if (SyncAuthenticPixels(image,exception) == MagickFalse)
               break;
          }
        }

        else /* Should not reach this; colormap already exists and
                must be <= 256 */
        {
          if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "    Quantizing the colormap to 4-4-4");

          for (i=0; i<image_colors; i++)
          {
            LBR04PacketRGB(image->colormap[i]);
          }
        }
        continue;
      }

    if (tried_333 == MagickFalse && (image_colors == 0 || image_colors > 256))
      {
        if (logging != MagickFalse)
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
               "    Quantizing the background color to 3-3-3");

        tried_333 = MagickTrue;

        LBR03PacketRGB(image->background_color);

        if (logging != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "    Quantizing the pixel colors to 3-3-3-1");

        if (image->colormap == NULL)
        {
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            q=GetAuthenticPixels(image,0,y,image->columns,1,exception);

            if (q == (Quantum *) NULL)
              break;

            for (x=0; x < (ssize_t) image->columns; x++)
            {
              if (GetPixelAlpha(image,q) == OpaqueAlpha)
                  LBR03RGB(q);
              q+=(ptrdiff_t) GetPixelChannels(image);
            }

            if (SyncAuthenticPixels(image,exception) == MagickFalse)
               break;
          }
        }

        else /* Should not reach this; colormap already exists and
                must be <= 256 */
        {
          if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "    Quantizing the colormap to 3-3-3-1");
          for (i=0; i<image_colors; i++)
          {
              LBR03PacketRGB(image->colormap[i]);
          }
        }
        continue;
      }

    if (tried_332 == MagickFalse && (image_colors == 0 || image_colors > 256))
      {
        if (logging != MagickFalse)
           (void) LogMagickEvent(CoderEvent,GetMagickModule(),
               "    Quantizing the background color to 3-3-2");

        tried_332 = MagickTrue;

        /* Red and green were already done so we only quantize the blue
         * channel
         */

        LBR02PacketBlue(image->background_color);

        if (logging != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "    Quantizing the pixel colors to 3-3-2-1");

        if (image->colormap == NULL)
        {
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            q=GetAuthenticPixels(image,0,y,image->columns,1,exception);

            if (q == (Quantum *) NULL)
              break;

            for (x=0; x < (ssize_t) image->columns; x++)
            {
              if (GetPixelAlpha(image,q) == OpaqueAlpha)
                  LBR02PixelBlue(q);
              q+=(ptrdiff_t) GetPixelChannels(image);
            }

            if (SyncAuthenticPixels(image,exception) == MagickFalse)
               break;
          }
        }

        else /* Should not reach this; colormap already exists and
                must be <= 256 */
        {
          if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "    Quantizing the colormap to 3-3-2-1");
          for (i=0; i<image_colors; i++)
          {
              LBR02PacketBlue(image->colormap[i]);
          }
      }
      continue;
    }

    if (image_colors == 0 || image_colors > 256)
    {
      /* Take care of special case with 256 opaque colors + 1 transparent
       * color.  We don't need to quantize to 2-3-2-1; we only need to
       * eliminate one color, so we'll merge the two darkest red
       * colors (0x49, 0, 0) -> (0x24, 0, 0).
       */
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Merging two dark red background colors to 3-3-2-1");