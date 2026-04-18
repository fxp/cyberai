// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 34/94



            p+=(ptrdiff_t) 8;
            mng_info->ticks_per_second=(size_t) mng_get_long(p);

            if (mng_info->ticks_per_second == 0)
              default_frame_delay=0;

            else
              default_frame_delay=(size_t) image->ticks_per_second/
                (size_t) mng_info->ticks_per_second;

            frame_delay=default_frame_delay;
            simplicity=0;

            p+=(ptrdiff_t) 16;
            simplicity=(size_t) mng_get_long(p);

            mng_type=1;    /* Full MNG */

            if ((simplicity != 0) && ((simplicity | 11) == 11))
              mng_type=2; /* LC */

            if ((simplicity != 0) && ((simplicity | 9) == 9))
              mng_type=3; /* VLC */

            if (mng_type != 3)
              insert_layers=MagickTrue;
            if (GetAuthenticPixelQueue(image) != (Quantum *) NULL)
              {
                /* Allocate next image structure.  */
                AcquireNextImage(image_info,image,exception);

                if (GetNextImageInList(image) == (Image *) NULL)
                  return((Image *) NULL);

                image=SyncNextImageInList(image);
                mng_info->image=image;
              }

            if ((mng_info->mng_width > 65535L) ||
                (mng_info->mng_height > 65535L))
              {
                chunk=(unsigned char *) RelinquishMagickMemory(chunk);
                ThrowReaderException(ImageError,"WidthOrHeightExceedsLimit");
              }

            (void) FormatLocaleString(page_geometry,MagickPathExtent,
              "%.20gx%.20g+0+0",(double) mng_info->mng_width,(double)
              mng_info->mng_height);

            mng_info->frame.left=0;
            mng_info->frame.right=(ssize_t) mng_info->mng_width;
            mng_info->frame.top=0;
            mng_info->frame.bottom=(ssize_t) mng_info->mng_height;
            mng_info->clip=default_fb=previous_fb=mng_info->frame;

            for (i=0; i < MNG_MAX_OBJECTS; i++)
              mng_info->object_clip[i]=mng_info->frame;

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }

        if (memcmp(type,mng_TERM,4) == 0)
          {
            int
              repeat=0;

            if (length != 0)
              repeat=p[0];

            if (repeat == 3 && length > 9)
              {
                final_delay=(png_uint_32) mng_get_long(&p[2]);
                mng_iterations=(png_uint_32) mng_get_long(&p[6]);

                if (mng_iterations == PNG_UINT_31_MAX)
                  mng_iterations=0;

                image->iterations=mng_iterations;
                term_chunk_found=MagickTrue;
              }

            if (logging != MagickFalse)
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "    repeat=%d,  final_delay=%.20g,  iterations=%.20g",
                  repeat,(double) final_delay, (double) image->iterations);
              }

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }
        if (memcmp(type,mng_DEFI,4) == 0)
          {
            if (mng_type == 3)
              {
                (void) ThrowMagickException(exception,GetMagickModule(),
                  CoderError,"DEFI chunk found in MNG-VLC datastream","`%s'",
                  image->filename);
                chunk=(unsigned char *) RelinquishMagickMemory(chunk);
                continue;
              }

            if (length < 2)
              {
                chunk=(unsigned char *) RelinquishMagickMemory(chunk);
                ThrowReaderException(CorruptImageError,"CorruptImage");
              }

            object_id=(int) (((unsigned int) p[0] << 8) | (unsigned int) p[1]);

            if (mng_type == 2 && object_id != 0)
              (void) ThrowMagickException(exception,GetMagickModule(),
                CoderError,"Nonzero object_id in MNG-LC datastream","`%s'",
                image->filename);

            if (object_id >= MNG_MAX_OBJECTS)
              {
                /*
                  Instead of using a warning we should allocate a larger
                  MngReadInfo structure and continue.
                */
                (void) ThrowMagickException(exception,GetMagickModule(),
                  CoderError,"object id too large","`%s'",image->filename);
                object_id=MNG_MAX_OBJECTS-1;
              }

            if (mng_info->exists[object_id])
              if (mng_info->frozen[object_id])
                {
                  chunk=(unsigned char *) RelinquishMagickMemory(chunk);
                  (void) ThrowMagickException(exception,
                    GetMagickModule(),CoderError,
                    "DEFI cannot redefine a frozen MNG object","`%s'",
                    image->filename);
                  continue;
                }

            mng_info->exists[object_id]=MagickTrue;