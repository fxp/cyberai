// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 62/94



      if (ScaleQuantumToChar((Quantum) image->background_color.red) == 0x49 &&
          ScaleQuantumToChar((Quantum) image->background_color.green) == 0x00 &&
          ScaleQuantumToChar((Quantum) image->background_color.blue) == 0x00)
      {
         image->background_color.red=ScaleCharToQuantum(0x24);
      }

      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Merging two dark red pixel colors to 3-3-2-1");

      if (image->colormap == NULL)
      {
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          q=GetAuthenticPixels(image,0,y,image->columns,1,exception);

          if (q == (Quantum *) NULL)
            break;

          for (x=0; x < (ssize_t) image->columns; x++)
          {
            if (ScaleQuantumToChar(GetPixelRed(image,q)) == 0x49 &&
                ScaleQuantumToChar(GetPixelGreen(image,q)) == 0x00 &&
                ScaleQuantumToChar(GetPixelBlue(image,q)) == 0x00 &&
                GetPixelAlpha(image,q) == OpaqueAlpha)
              {
                SetPixelRed(image,ScaleCharToQuantum(0x24),q);
              }
            q+=(ptrdiff_t) GetPixelChannels(image);
          }

          if (SyncAuthenticPixels(image,exception) == MagickFalse)
             break;

        }
      }

      else
      {
         for (i=0; i<image_colors; i++)
         {
            if (ScaleQuantumToChar((Quantum) image->colormap[i].red) == 0x49 &&
                ScaleQuantumToChar((Quantum) image->colormap[i].green) == 0x00 &&
                ScaleQuantumToChar((Quantum) image->colormap[i].blue) == 0x00)
            {
               image->colormap[i].red=ScaleCharToQuantum(0x24);
            }
         }
      }
    }
  }
  }
  /* END OF BUILD_PALETTE */

  /* If we are excluding the tRNS chunk and there is transparency,
   * then we must write a Gray-Alpha (color-type 4) or RGBA (color-type 6)
   * PNG.
   */
  if (mng_info->exclude_tRNS != MagickFalse &&
     (number_transparent != 0 || number_semitransparent != 0))
    {
      volatile unsigned int colortype = mng_info->colortype;

      if (ping_have_color == MagickFalse)
        mng_info->colortype=5;

      else
        mng_info->colortype=7;

      if (colortype != 0 &&
         mng_info->colortype != colortype)
        ping_need_colortype_warning=MagickTrue;

    }

  /* See if cheap transparency is possible.  It is only possible
   * when there is a single transparent color, no semitransparent
   * color, and no opaque color that has the same RGB components
   * as the transparent color.  We only need this information if
   * we are writing a PNG with colortype 0 or 2, and we have not
   * excluded the tRNS chunk.
   */
  if (number_transparent == 1 &&
      mng_info->colortype < 4)
    {
       ping_have_cheap_transparency = MagickTrue;

       if (number_semitransparent != 0)
         ping_have_cheap_transparency = MagickFalse;

       else if (image_colors == 0 || image_colors > 256 ||
           image->colormap == NULL)
         {
           const Quantum
             *q;

           for (y=0; y < (ssize_t) image->rows; y++)
           {
             q=GetVirtualPixels(image,0,y,image->columns,1, exception);

             if (q == (Quantum *) NULL)
               break;

             for (x=0; x < (ssize_t) image->columns; x++)
             {
                 if (GetPixelAlpha(image,q) != TransparentAlpha &&
                     (unsigned short) GetPixelRed(image,q) ==
                                     ping_trans_color.red &&
                     (unsigned short) GetPixelGreen(image,q) ==
                                     ping_trans_color.green &&
                     (unsigned short) GetPixelBlue(image,q) ==
                                     ping_trans_color.blue)
                   {
                     ping_have_cheap_transparency = MagickFalse;
                     break;
                   }

                 q+=(ptrdiff_t) GetPixelChannels(image);
             }

             if (ping_have_cheap_transparency == MagickFalse)
                break;
           }
         }
       else
         {
            /* Assuming that image->colormap[0] is the one transparent color
             * and that all others are opaque.
             */
            if (image_colors > 1)
              for (i=1; i<image_colors; i++)
                if (image->colormap[i].red == image->colormap[0].red &&
                    image->colormap[i].green == image->colormap[0].green &&
                    image->colormap[i].blue == image->colormap[0].blue)
                  {
                     ping_have_cheap_transparency = MagickFalse;
                     break;
                  }
         }

       if (logging != MagickFalse)
         {
           if (ping_have_cheap_transparency == MagickFalse)
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "   Cheap transparency is not possible.");