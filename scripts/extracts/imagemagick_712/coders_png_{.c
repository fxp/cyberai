// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 59/94



               /* Worst case is black-and-white; we are looking at every
                * pixel twice.
                */

               if (ping_have_non_bw == MagickFalse)
                 {
                   r=q;
                   for (x=0; x < (ssize_t) image->columns; x++)
                   {
                     if (GetPixelRed(image,r) != 0 &&
                         GetPixelRed(image,r) != QuantumRange)
                       {
                         ping_have_non_bw=MagickTrue;
                         break;
                       }
                     r+=(ptrdiff_t) GetPixelChannels(image);
                   }
               }
             }
           }
       }

     if (image_colors < 257)
       {
         PixelInfo
           colormap[260];

         /*
          * Initialize image colormap.
          */

         if (logging != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "      Sort the new colormap");

        /* Sort palette, transparent first */;

         n = 0;

         for (i=0; i<number_transparent; i++)
            colormap[n++] = transparent[i];

         for (i=0; i<number_semitransparent; i++)
            colormap[n++] = semitransparent[i];

         for (i=0; i<number_opaque; i++)
            colormap[n++] = opaque[i];

         ping_background.index += (png_byte) (number_transparent +
           number_semitransparent);

         /* image_colors < 257; search the colormap instead of the pixels
          * to get ping_have_color and ping_have_non_bw
          */
         for (i=0; i<n; i++)
         {
           if (ping_have_color == MagickFalse)
             {
                if (colormap[i].red != colormap[i].green ||
                    colormap[i].red != colormap[i].blue)
                  {
                     ping_have_color=MagickTrue;
                     ping_have_non_bw=MagickTrue;
                     break;
                  }
              }

           if (ping_have_non_bw == MagickFalse)
             {
               if (colormap[i].red != 0 && colormap[i].red != (double) QuantumRange)
                   ping_have_non_bw=MagickTrue;
             }
          }

        if ((mng_info->exclude_tRNS == MagickFalse ||
            (number_transparent == 0 && number_semitransparent == 0)) &&
            (((mng_info->colortype-1) == PNG_COLOR_TYPE_PALETTE) ||
            (mng_info->colortype == 0)))
          {
            if (logging != MagickFalse)
              {
                if (n !=  (ssize_t) image_colors)
                   (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "   image_colors (%d) and n (%d)  don't match",
                   image_colors, n);

                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "      AcquireImageColormap");
              }

            image->colors = (size_t) image_colors;

            if (AcquireImageColormap(image,(size_t) image_colors,exception) == MagickFalse)
              {
                (void) ThrowMagickException(exception,GetMagickModule(),
                  ResourceLimitError,"MemoryAllocationFailed","`%s'",
                  image->filename);
                break;
              }

            for (i=0; i< (ssize_t) image_colors; i++)
               image->colormap[i] = colormap[i];

            if (logging != MagickFalse)
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                      "      image->colors=%d (%d)",
                      (int) image->colors, image_colors);

                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                      "      Update the pixel indexes");
              }

            /* Sync the pixel indices with the new colormap */

            for (y=0; y < (ssize_t) image->rows; y++)
            {
              q=GetAuthenticPixels(image,0,y,image->columns,1,exception);

              if (q == (Quantum *) NULL)
                break;

              for (x=0; x < (ssize_t) image->columns; x++)
              {
                for (i=0; i< (ssize_t) image_colors; i++)
                {
                  if (((((image->alpha_trait & BlendPixelTrait) == 0) &&
                       (((image->alpha_trait & UpdatePixelTrait) == 0))) ||
                      image->colormap[i].alpha == (double) GetPixelAlpha(image,q)) &&
                      image->colormap[i].red == (double) GetPixelRed(image,q) &&
                      image->colormap[i].green == (double) GetPixelGreen(image,q) &&
                      image->colormap[i].blue == (double) GetPixelBlue(image,q))
                  {
                    SetPixelIndex(image,(Quantum) i,q);
                    break;
                  }
                }
                q+=(ptrdiff_t) GetPixelChannels(image);
              }

              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                 break;
            }
          }
       }