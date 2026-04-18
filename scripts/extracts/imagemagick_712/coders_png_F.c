// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 38/94



        if ((memcmp(type,mng_DISC,4) == 0) || (memcmp(type,mng_SEEK,4) == 0))
          {
            /* Read DISC or SEEK.  */

            if ((length == 0) || (length % 2) || !memcmp(type,mng_SEEK,4))
              {
                for (i=1; i < MNG_MAX_OBJECTS; i++)
                  MngReadInfoDiscardObject(mng_info,(int) i);
              }

            else
              {
                ssize_t
                  j;

                for (j=1; j < (ssize_t) length; j+=2)
                {
                  i=p[j-1] << 8 | p[j];
                  MngReadInfoDiscardObject(mng_info,(int) i);
                }
              }

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);

            continue;
          }

        if (memcmp(type,mng_MOVE,4) == 0)
          {
            size_t
              first_object,
              last_object;

            /* read MOVE */

            if (length > 3)
            {
              first_object=(unsigned int) ((p[0] << 8) | p[1]);
              last_object=(unsigned int) ((p[2] << 8) | p[3]);
              p+=(ptrdiff_t) 4;

              for (i=(ssize_t) first_object; i <= (ssize_t) last_object; i++)
              {
                if ((i < 0) || (i >= MNG_MAX_OBJECTS))
                  continue;

                if (mng_info->exists[i] && !mng_info->frozen[i] &&
                    (p-chunk) < (ssize_t) (length-8))
                  {
                    MngPair
                      new_pair;

                    MngPair
                      old_pair;

                    old_pair.a=(volatile long) mng_info->x_off[i];
                    old_pair.b=(volatile long) mng_info->y_off[i];
                    new_pair=mng_read_pair(old_pair,(int) p[0],&p[1]);
                    mng_info->x_off[i]=new_pair.a;
                    mng_info->y_off[i]=new_pair.b;
                  }
              }
            }

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }

        if (memcmp(type,mng_LOOP,4) == 0)
          {
            ssize_t loop_iters=1;
            if (length > 4)
              {
                loop_level=chunk[0];
                mng_info->loop_active[loop_level]=1;  /* mark loop active */

                /* Record starting point.  */
                loop_iters=mng_get_long(&chunk[1]);

                if (logging != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "  LOOP level %.20g has %.20g iterations ",
                    (double) loop_level, (double) loop_iters);

                if (loop_iters <= 0)
                  skipping_loop=(volatile short) loop_level;

                else
                  {
                    if ((MagickSizeType) loop_iters > GetMagickResourceLimit(ListLengthResource))
                      loop_iters=(ssize_t) GetMagickResourceLimit(ListLengthResource);
                    if (loop_iters >= 2147483647L)
                      loop_iters=2147483647L;
                    if (image_info->number_scenes != 0)
                      if (loop_iters > (ssize_t) image_info->number_scenes)
                        loop_iters=(ssize_t) image_info->number_scenes;
                    mng_info->loop_jump[loop_level]=TellBlob(image);
                    mng_info->loop_count[loop_level]=loop_iters;
                  }

                mng_info->loop_iteration[loop_level]=0;
              }
            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }

        if (memcmp(type,mng_ENDL,4) == 0)
          {
            if (length > 0)
              {
                loop_level=chunk[0];

                if (skipping_loop > 0)
                  {
                    if (skipping_loop == loop_level)
                      {
                        /*
                          Found end of zero-iteration loop.
                        */
                        skipping_loop=(-1);
                        mng_info->loop_active[loop_level]=0;
                      }
                  }

                else
                  {
                    if (mng_info->loop_active[loop_level] == 1)
                      {
                        mng_info->loop_count[loop_level]--;
                        mng_info->loop_iteration[loop_level]++;

                        if (logging != MagickFalse)
                          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                          "  ENDL: LOOP level %.20g has %.20g remaining iters",
                            (double) loop_level,(double)
                            mng_info->loop_count[loop_level]);

                        if (mng_info->loop_count[loop_level] > 0)
                          {
                            offset=
                              SeekBlob(image,mng_info->loop_jump[loop_level],
                              SEEK_SET);