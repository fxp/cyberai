// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 93/94



         (void) WriteBlob(image,data_length+4,chunk);
         (void) WriteBlobMSBULong(image,crc32(0,chunk,(uInt) (data_length+4)));
         mng_info->have_global_plte=MagickTrue;
       }
    }
  scene=0;
  mng_info->delay=0;
  mng_info->equal_palettes=MagickFalse;
  number_scenes=GetImageListLength(image);
  do
  {
    if (mng_info->adjoin != MagickFalse)
    {
    /*
      If we aren't using a global palette for the entire MNG, check to
      see if we can use one for two or more consecutive images.
    */
    if (need_local_plte && use_global_plte && !all_images_are_gray)
      {
        if (mng_info->is_palette != MagickFalse)
          {
            /*
              When equal_palettes is true, this image has the same palette
              as the previous PseudoClass image
            */
            mng_info->have_global_plte=mng_info->equal_palettes;
            mng_info->equal_palettes=PalettesAreEqual(image,image->next);
            if ((mng_info->equal_palettes && !mng_info->have_global_plte) &&
                (image->colors <= 256))
              {
                /*
                  Write MNG PLTE chunk
                */
                size_t
                  data_length;

                ssize_t
                  i;

                data_length=3*image->colors;
                (void) WriteBlobMSBULong(image,data_length);
                PNGType(chunk,mng_PLTE);
                LogPNGChunk(logging,mng_PLTE,data_length);

                for (i=0; i < (ssize_t) image->colors; i++)
                {
                  chunk[4+i*3]=ScaleQuantumToChar((Quantum) image->colormap[i].red);
                  chunk[5+i*3]=ScaleQuantumToChar((Quantum) image->colormap[i].green);
                  chunk[6+i*3]=ScaleQuantumToChar((Quantum) image->colormap[i].blue);
                }

                (void) WriteBlob(image,data_length+4,chunk);
                (void) WriteBlobMSBULong(image,crc32(0,chunk,
                   (uInt) (data_length+4)));
                mng_info->have_global_plte=MagickTrue;
              }
          }
        else
          mng_info->have_global_plte=MagickFalse;
      }
    if (mng_info->need_defi)
      {
        ssize_t
          previous_x,
          previous_y;

        if (scene != 0)
          {
            previous_x=mng_info->page.x;
            previous_y=mng_info->page.y;
          }
        else
          {
            previous_x=0;
            previous_y=0;
          }
        mng_info->page=image->page;
        if ((mng_info->page.x !=  previous_x) ||
            (mng_info->page.y != previous_y))
          {
             (void) WriteBlobMSBULong(image,12L);  /* data length=12 */
             PNGType(chunk,mng_DEFI);
             LogPNGChunk(logging,mng_DEFI,12L);
             chunk[4]=0; /* object 0 MSB */
             chunk[5]=0; /* object 0 LSB */
             chunk[6]=0; /* visible  */
             chunk[7]=0; /* abstract */
             PNGLong(chunk+8,(png_uint_32) mng_info->page.x);
             PNGLong(chunk+12,(png_uint_32) mng_info->page.y);
             (void) WriteBlob(image,16,chunk);
             (void) WriteBlobMSBULong(image,crc32(0,chunk,16));
          }
      }
    }

   mng_info->write_mng=write_mng;

   if ((int) image->dispose >= BackgroundDispose)
     mng_info->framing_mode=3;

   if (mng_info->need_fram != MagickFalse && mng_info->adjoin != MagickFalse &&
       ((image->delay != mng_info->delay) ||
        (mng_info->framing_mode != mng_info->old_framing_mode)))
     {
       if (image->delay == mng_info->delay)
         {
           /*
             Write a MNG FRAM chunk with the new framing mode.
           */
           (void) WriteBlobMSBULong(image,1L);  /* data length=1 */
           PNGType(chunk,mng_FRAM);
           LogPNGChunk(logging,mng_FRAM,1L);
           chunk[4]=(unsigned char) mng_info->framing_mode;
           (void) WriteBlob(image,5,chunk);
           (void) WriteBlobMSBULong(image,crc32(0,chunk,5));
         }
       else
         {
           /*
             Write a MNG FRAM chunk with the delay.
           */
           (void) WriteBlobMSBULong(image,10L);  /* data length=10 */
           PNGType(chunk,mng_FRAM);
           LogPNGChunk(logging,mng_FRAM,10L);
           chunk[4]=(unsigned char) mng_info->framing_mode;
           chunk[5]=0;  /* frame name separator (no name) */
           chunk[6]=2;  /* flag for changing default delay */
           chunk[7]=0;  /* flag for changing frame timeout */
           chunk[8]=0;  /* flag for changing frame clipping */
           chunk[9]=0;  /* flag for changing frame sync_id */
           PNGLong(chunk+10,(png_uint_32) ((mng_info->ticks_per_second*
             image->delay)/(size_t) MagickMax(image->ticks_per_second,1)));
           (void) WriteBlob(image,14,chunk);
           (void) WriteBlobMSBULong(image,crc32(0,chunk,14));
           mng_info->delay=(png_uint_32) image->delay;
         }
       mng_info->old_framing_mode=mng_info->framing_mode;
     }