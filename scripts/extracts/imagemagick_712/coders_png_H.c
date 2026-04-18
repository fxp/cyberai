// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 40/94



        if (memcmp(type,mng_SHOW,4) == 0)
          {
            if (mng_info->show_warning == 0)
              (void) ThrowMagickException(exception,GetMagickModule(),
                CoderError,"SHOW is not implemented yet","`%s'",
                image->filename);

            mng_info->show_warning++;
          }

        if (memcmp(type,mng_sBIT,4) == 0)
          {
            if (length < 4)
              mng_info->have_global_sbit=MagickFalse;

            else
              {
                mng_info->global_sbit.gray=p[0];
                mng_info->global_sbit.red=p[0];
                mng_info->global_sbit.green=p[1];
                mng_info->global_sbit.blue=p[2];
                mng_info->global_sbit.alpha=p[3];
                mng_info->have_global_sbit=MagickTrue;
             }
          }
        if (memcmp(type,mng_pHYs,4) == 0)
          {
            if (length > 8)
              {
                mng_info->global_x_pixels_per_unit=
                    (size_t) mng_get_long(p);
                mng_info->global_y_pixels_per_unit=
                    (size_t) mng_get_long(&p[4]);
                mng_info->global_phys_unit_type=p[8];
                mng_info->have_global_phys=MagickTrue;
              }

            else
              mng_info->have_global_phys=MagickFalse;
          }
        if (memcmp(type,mng_pHYg,4) == 0)
          {
            if (mng_info->phyg_warning == 0)
              (void) ThrowMagickException(exception,GetMagickModule(),
                CoderError,"pHYg is not implemented.","`%s'",image->filename);

            mng_info->phyg_warning++;
          }
        if (memcmp(type,mng_BASI,4) == 0)
          {
            skip_to_iend=MagickTrue;

            if (mng_info->basi_warning == 0)
              (void) ThrowMagickException(exception,GetMagickModule(),
                CoderError,"BASI is not implemented yet","`%s'",
                image->filename);

            mng_info->basi_warning++;
            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }

        if (memcmp(type,mng_IHDR,4)
#if defined(JNG_SUPPORTED)
            && memcmp(type,mng_JHDR,4)
#endif
            )
          {
            /* Not an IHDR or JHDR chunk */
            chunk=(unsigned char *) RelinquishMagickMemory(chunk);

            continue;
          }
/* Process IHDR */
        if (logging != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "  Processing %c%c%c%c chunk",type[0],type[1],type[2],type[3]);

        mng_info->exists[object_id]=MagickTrue;
        mng_info->viewable[object_id]=MagickTrue;

        if (mng_info->invisible[object_id])
          {
            if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "  Skipping invisible object");

            skip_to_iend=MagickTrue;
            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }
        if (length < 8)
          {
            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            ThrowReaderException(CorruptImageError,"ImproperImageHeader");
          }

        image_width=(size_t) mng_get_long(p);
        image_height=(size_t) mng_get_long(&p[4]);
        chunk=(unsigned char *) RelinquishMagickMemory(chunk);

        /*
          Insert a transparent background layer behind the entire animation
          if it is not full screen.
        */
        if (insert_layers && mng_type && first_mng_object)
          {
            if ((mng_info->clip.left > 0) || (mng_info->clip.top > 0) ||
                (image_width < mng_info->mng_width) ||
                (mng_info->clip.right < (ssize_t) mng_info->mng_width) ||
                (image_height < mng_info->mng_height) ||
                (mng_info->clip.bottom < (ssize_t) mng_info->mng_height))
              {
                if (GetAuthenticPixelQueue(image) != (Quantum *) NULL)
                  {
                    /*
                      Allocate next image structure.
                    */
                    AcquireNextImage(image_info,image,exception);

                    if (GetNextImageInList(image) == (Image *) NULL)
                      return(DestroyImageList(image));

                    image=SyncNextImageInList(image);
                  }
                mng_info->image=image;

                if (term_chunk_found)
                  {
                    image->start_loop=MagickTrue;
                    image->iterations=mng_iterations;
                    term_chunk_found=MagickFalse;
                  }

                else
                    image->start_loop=MagickFalse;

                /* Make a background rectangle.  */