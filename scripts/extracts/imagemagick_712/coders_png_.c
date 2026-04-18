// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 92/94



             /*
                Write MNG cHRM chunk
             */
             (void) WriteBlobMSBULong(image,32L);
             PNGType(chunk,mng_cHRM);
             LogPNGChunk(logging,mng_cHRM,32L);
             primary=image->chromaticity.white_point;
             PNGLong(chunk+4,(png_uint_32) (100000*primary.x+0.5));
             PNGLong(chunk+8,(png_uint_32) (100000*primary.y+0.5));
             primary=image->chromaticity.red_primary;
             PNGLong(chunk+12,(png_uint_32) (100000*primary.x+0.5));
             PNGLong(chunk+16,(png_uint_32) (100000*primary.y+0.5));
             primary=image->chromaticity.green_primary;
             PNGLong(chunk+20,(png_uint_32) (100000*primary.x+0.5));
             PNGLong(chunk+24,(png_uint_32) (100000*primary.y+0.5));
             primary=image->chromaticity.blue_primary;
             PNGLong(chunk+28,(png_uint_32) (100000*primary.x+0.5));
             PNGLong(chunk+32,(png_uint_32) (100000*primary.y+0.5));
             (void) WriteBlob(image,36,chunk);
             (void) WriteBlobMSBULong(image,crc32(0,chunk,36));
             mng_info->have_global_chrm=MagickTrue;
           }
       }
     if (image->resolution.x && image->resolution.y &&
         mng_info->equal_physs == MagickFalse)
       {
         /*
            Write MNG pHYs chunk
         */
         (void) WriteBlobMSBULong(image,9L);
         PNGType(chunk,mng_pHYs);
         LogPNGChunk(logging,mng_pHYs,9L);

         if (image->units == PixelsPerInchResolution)
           {
             PNGLong(chunk+4,(png_uint_32)
               (image->resolution.x*100.0/2.54+0.5));

             PNGLong(chunk+8,(png_uint_32)
               (image->resolution.y*100.0/2.54+0.5));

             chunk[12]=1;
           }

         else
           {
             if (image->units == PixelsPerCentimeterResolution)
               {
                 PNGLong(chunk+4,(png_uint_32)
                   (image->resolution.x*100.0+0.5));

                 PNGLong(chunk+8,(png_uint_32)
                   (image->resolution.y*100.0+0.5));

                 chunk[12]=1;
               }

             else
               {
                 PNGLong(chunk+4,(png_uint_32) (image->resolution.x+0.5));
                 PNGLong(chunk+8,(png_uint_32) (image->resolution.y+0.5));
                 chunk[12]=0;
               }
           }
         (void) WriteBlob(image,13,chunk);
         (void) WriteBlobMSBULong(image,crc32(0,chunk,13));
       }
     /*
       Write MNG BACK chunk and global bKGD chunk, if the image is transparent
       or does not cover the entire frame.
     */
     if ((write_mng != MagickFalse) && ((image->alpha_trait != UndefinedPixelTrait) ||
         image->page.x > 0 || image->page.y > 0 || (image->page.width &&
         ((ssize_t) image->page.width+image->page.x < (ssize_t) mng_info->page.width))
         || (image->page.height && ((ssize_t) image->page.height+image->page.y
         < (ssize_t) mng_info->page.height))))
       {
         (void) WriteBlobMSBULong(image,6L);
         PNGType(chunk,mng_BACK);
         LogPNGChunk(logging,mng_BACK,6L);
         red=ScaleQuantumToShort((Quantum) image->background_color.red);
         green=ScaleQuantumToShort((Quantum) image->background_color.green);
         blue=ScaleQuantumToShort((Quantum) image->background_color.blue);
         PNGShort(chunk+4,red);
         PNGShort(chunk+6,green);
         PNGShort(chunk+8,blue);
         (void) WriteBlob(image,10,chunk);
         (void) WriteBlobMSBULong(image,crc32(0,chunk,10));
         if (mng_info->equal_backgrounds != MagickFalse)
           {
             (void) WriteBlobMSBULong(image,6L);
             PNGType(chunk,mng_bKGD);
             LogPNGChunk(logging,mng_bKGD,6L);
             (void) WriteBlob(image,10,chunk);
             (void) WriteBlobMSBULong(image,crc32(0,chunk,10));
           }
       }

     if ((need_local_plte == MagickFalse) &&
         (image->storage_class == PseudoClass) &&
         (all_images_are_gray == MagickFalse) && (image->colors <= 256))
       {
         size_t
           data_length;

         ssize_t
           i;

         /*
           Write MNG PLTE chunk
         */
         data_length=3*image->colors;
         (void) WriteBlobMSBULong(image,data_length);
         PNGType(chunk,mng_PLTE);
         LogPNGChunk(logging,mng_PLTE,data_length);

         for (i=0; i < (ssize_t) image->colors; i++)
         {
           chunk[4+i*3]=(unsigned char) (ScaleQuantumToChar(
             (Quantum) image->colormap[i].red) & 0xff);
           chunk[5+i*3]=(unsigned char) (ScaleQuantumToChar(
             (Quantum) image->colormap[i].green) & 0xff);
           chunk[6+i*3]=(unsigned char) (ScaleQuantumToChar(
             (Quantum) image->colormap[i].blue) & 0xff);
         }