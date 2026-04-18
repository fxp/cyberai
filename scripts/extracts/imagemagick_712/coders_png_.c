// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 91/94



     else
       {
         if (need_matte)
           {
             if (mng_info->need_defi || mng_info->need_fram != MagickFalse || use_global_plte)
               PNGLong(chunk+28,11L);    /* simplicity=LC */

             else
               PNGLong(chunk+28,9L);    /* simplicity=VLC */
           }

         else
           {
             if (mng_info->need_defi || mng_info->need_fram != MagickFalse || use_global_plte)
               PNGLong(chunk+28,3L);    /* simplicity=LC, no transparency */

             else
               PNGLong(chunk+28,1L);    /* simplicity=VLC, no transparency */
           }
       }
     (void) WriteBlob(image,32,chunk);
     (void) WriteBlobMSBULong(image,crc32(0,chunk,32));
     option=GetImageOption(image_info,"mng:need-cacheoff");
     if (option != (const char *) NULL)
       {
         size_t
           length;
         /*
           Write "nEED CACHEOFF" to turn playback caching off for streaming MNG.
         */
         PNGType(chunk,mng_nEED);
         length=CopyMagickString((char *) chunk+4,"CACHEOFF",20);
         (void) WriteBlobMSBULong(image,(size_t) length);
         LogPNGChunk(logging,mng_nEED,(size_t) length);
         length+=4;
         (void) WriteBlob(image,length,chunk);
         (void) WriteBlobMSBULong(image,crc32(0,chunk,(uInt) length));
       }
     if ((GetPreviousImageInList(image) == (Image *) NULL) &&
         (GetNextImageInList(image) != (Image *) NULL) &&
         (image->iterations != 1))
       {
         /*
           Write MNG TERM chunk
         */
         (void) WriteBlobMSBULong(image,10L);  /* data length=10 */
         PNGType(chunk,mng_TERM);
         LogPNGChunk(logging,mng_TERM,10L);
         chunk[4]=3;  /* repeat animation */
         chunk[5]=0;  /* show last frame when done */
         PNGLong(chunk+6,(png_uint_32) (mng_info->ticks_per_second*
            final_delay/(size_t) MagickMax(image->ticks_per_second,1)));

         if (image->iterations == 0)
           PNGLong(chunk+10,PNG_UINT_31_MAX);

         else
           PNGLong(chunk+10,(png_uint_32) image->iterations);

         if (logging != MagickFalse)
           {
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
               "     TERM delay: %.20g",(double) (mng_info->ticks_per_second*
              final_delay/(size_t) MagickMax(image->ticks_per_second,1)));

             if (image->iterations == 0)
               (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "     TERM iterations: %.20g",(double) PNG_UINT_31_MAX);

             else
               (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "     Image iterations: %.20g",(double) image->iterations);
           }
         (void) WriteBlob(image,14,chunk);
         (void) WriteBlobMSBULong(image,crc32(0,chunk,14));
       }
     /*
       To do: check for cHRM+gAMA == sRGB, and write sRGB instead.
     */
     if ((image->colorspace == sRGBColorspace || image->rendering_intent) &&
          mng_info->equal_srgbs != MagickFalse)
       {
         /*
           Write MNG sRGB chunk
         */
         (void) WriteBlobMSBULong(image,1L);
         PNGType(chunk,mng_sRGB);
         LogPNGChunk(logging,mng_sRGB,1L);

         if (image->rendering_intent != UndefinedIntent)
           chunk[4]=(unsigned char)
             Magick_RenderingIntent_to_PNG_RenderingIntent(
             (image->rendering_intent));

         else
           chunk[4]=(unsigned char)
             Magick_RenderingIntent_to_PNG_RenderingIntent(
               (PerceptualIntent));

         (void) WriteBlob(image,5,chunk);
         (void) WriteBlobMSBULong(image,crc32(0,chunk,5));
         mng_info->have_global_srgb=MagickTrue;
       }

     else
       {
         if (image->gamma && mng_info->equal_gammas != MagickFalse)
           {
             /*
                Write MNG gAMA chunk
             */
             (void) WriteBlobMSBULong(image,4L);
             PNGType(chunk,mng_gAMA);
             LogPNGChunk(logging,mng_gAMA,4L);
             PNGLong(chunk+4,(png_uint_32) (100000*image->gamma+0.5));
             (void) WriteBlob(image,8,chunk);
             (void) WriteBlobMSBULong(image,crc32(0,chunk,8));
             mng_info->have_global_gama=MagickTrue;
           }
         if (mng_info->equal_chrms != MagickFalse)
           {
             PrimaryInfo
               primary;