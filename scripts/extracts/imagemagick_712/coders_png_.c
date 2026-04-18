// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 90/94



            if (mng_info->equal_chrms != MagickFalse)
              {
                if (next_image->chromaticity.red_primary.x !=
                    next_image->next->chromaticity.red_primary.x ||
                    next_image->chromaticity.red_primary.y !=
                    next_image->next->chromaticity.red_primary.y ||
                    next_image->chromaticity.green_primary.x !=
                    next_image->next->chromaticity.green_primary.x ||
                    next_image->chromaticity.green_primary.y !=
                    next_image->next->chromaticity.green_primary.y ||
                    next_image->chromaticity.blue_primary.x !=
                    next_image->next->chromaticity.blue_primary.x ||
                    next_image->chromaticity.blue_primary.y !=
                    next_image->next->chromaticity.blue_primary.y ||
                    next_image->chromaticity.white_point.x !=
                    next_image->next->chromaticity.white_point.x ||
                    next_image->chromaticity.white_point.y !=
                    next_image->next->chromaticity.white_point.y)
                  mng_info->equal_chrms=MagickFalse;
              }
          }
        image_count++;
        next_image=GetNextImageInList(next_image);
      }
      if (image_count < 2)
        {
          mng_info->equal_backgrounds=MagickFalse;
          mng_info->equal_chrms=MagickFalse;
          mng_info->equal_gammas=MagickFalse;
          mng_info->equal_srgbs=MagickFalse;
          mng_info->equal_physs=MagickFalse;
          use_global_plte=MagickFalse;
          need_local_plte=MagickTrue;
          need_iterations=MagickFalse;
        }

     if (mng_info->need_fram == MagickFalse)
       {
         /*
           Only certain framing rates 100/n are exactly representable without
           the FRAM chunk but we'll allow some slop in VLC files
         */
         if (final_delay == 0)
           {
             if (need_iterations != MagickFalse)
               {
                 /*
                   It's probably a GIF with loop; don't run it *too* fast.
                 */
                 if (mng_info->adjoin != MagickFalse)
                   {
                     final_delay=10;
                     (void) ThrowMagickException(exception,GetMagickModule(),
                       CoderWarning,
                       "input has zero delay between all frames; assuming",
                       " 10 cs `%s'","");
                   }
               }
             else
               mng_info->ticks_per_second=0;
           }
         if (final_delay != 0)
           mng_info->ticks_per_second=(png_uint_32)
              image->ticks_per_second/(png_uint_32) final_delay;
         if (final_delay > 50)
           mng_info->ticks_per_second=2;

         if (final_delay > 75)
           mng_info->ticks_per_second=1;

         if (final_delay > 125)
           mng_info->need_fram=MagickTrue;

         if (mng_info->need_defi && final_delay > 2 && (final_delay != 4) &&
            (final_delay != 5) && (final_delay != 10) && (final_delay != 20) &&
            (final_delay != 25) && (final_delay != 50) &&
            (final_delay != (size_t) image->ticks_per_second))
           mng_info->need_fram=MagickTrue;  /* make it exact; cannot be VLC */
       }

     if (mng_info->need_fram != MagickFalse)
        mng_info->ticks_per_second=(unsigned long) image->ticks_per_second;
     /*
        If pseudocolor, we should also check to see if all the
        palettes are identical and write a global PLTE if they are.
        ../glennrp Feb 99.
     */
     /*
        Write the MNG version 1.0 signature and MHDR chunk.
     */
     (void) WriteBlob(image,8,(const unsigned char *) "\212MNG\r\n\032\n");
     (void) WriteBlobMSBULong(image,28L);  /* chunk data length=28 */
     PNGType(chunk,mng_MHDR);
     LogPNGChunk(logging,mng_MHDR,28L);
     PNGLong(chunk+4,(png_uint_32) mng_info->page.width);
     PNGLong(chunk+8,(png_uint_32) mng_info->page.height);
     PNGLong(chunk+12,mng_info->ticks_per_second);
     PNGLong(chunk+16,0L);  /* layer count=unknown */
     PNGLong(chunk+20,0L);  /* frame count=unknown */
     PNGLong(chunk+24,0L);  /* play time=unknown   */
     if (write_jng != MagickFalse)
       {
         if (need_matte)
           {
             if (mng_info->need_defi || mng_info->need_fram != MagickFalse || use_global_plte)
               PNGLong(chunk+28,27L);    /* simplicity=LC+JNG */

             else
               PNGLong(chunk+28,25L);    /* simplicity=VLC+JNG */
           }

         else
           {
             if (mng_info->need_defi || mng_info->need_fram != MagickFalse || use_global_plte)
               PNGLong(chunk+28,19L);  /* simplicity=LC+JNG, no transparency */

             else
               PNGLong(chunk+28,17L);  /* simplicity=VLC+JNG, no transparency */
           }
       }