// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 36/94



            if (length == 32)
              {
                mng_info->global_chrm.white_point.x=0.00001*mng_get_long(p);
                mng_info->global_chrm.white_point.y=0.00001*mng_get_long(&p[4]);
                mng_info->global_chrm.red_primary.x=0.00001*mng_get_long(&p[8]);
                mng_info->global_chrm.red_primary.y=0.00001*
                  mng_get_long(&p[12]);
                mng_info->global_chrm.green_primary.x=0.00001*
                  mng_get_long(&p[16]);
                mng_info->global_chrm.green_primary.y=0.00001*
                  mng_get_long(&p[20]);
                mng_info->global_chrm.blue_primary.x=0.00001*
                  mng_get_long(&p[24]);
                mng_info->global_chrm.blue_primary.y=0.00001*
                  mng_get_long(&p[28]);
                mng_info->have_global_chrm=MagickTrue;
              }
            else
              mng_info->have_global_chrm=MagickFalse;

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }

        if (memcmp(type,mng_sRGB,4) == 0)
          {
            /*
              Read global sRGB.
            */
            if (length != 0)
              {
                mng_info->global_srgb_intent=
                  Magick_RenderingIntent_from_PNG_RenderingIntent(p[0]);
                mng_info->have_global_srgb=MagickTrue;
              }
            else
              mng_info->have_global_srgb=MagickFalse;

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }

        if (memcmp(type,mng_iCCP,4) == 0)
          {
            /* To do: */

            /*
              Read global iCCP.
            */
            chunk=(unsigned char *) RelinquishMagickMemory(chunk);

            continue;
          }

        if (memcmp(type,mng_FRAM,4) == 0)
          {
            if (mng_type == 3)
              (void) ThrowMagickException(exception,GetMagickModule(),
                CoderError,"FRAM chunk found in MNG-VLC datastream","`%s'",
                image->filename);

            if ((mng_info->framing_mode == 2) || (mng_info->framing_mode == 4))
              image->delay=frame_delay;

            frame_delay=default_frame_delay;
            frame_timeout=default_frame_timeout;
            fb=default_fb;

            if (length != 0)
              if (p[0])
                mng_info->framing_mode=p[0];

            if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "    Framing_mode=%d",mng_info->framing_mode);

            if (length > 6)
              {
                /* Note the delay and frame clipping boundaries.  */

                p++; /* framing mode */

                while (((p-chunk) < (long) length) && *p)
                  p++;  /* frame name */

                p++;  /* frame name terminator */

                if ((p-chunk) < (ssize_t) (length-4))
                  {
                    int
                      change_delay,
                      change_timeout,
                      change_clipping;

                    change_delay=(*p++);
                    change_timeout=(*p++);
                    change_clipping=(*p++);
                    p++; /* change_sync */

                    if (change_delay && ((p-chunk)+4 <= (ssize_t) length))
                      {
                        frame_delay=(size_t) image->ticks_per_second*
                          (size_t) mng_get_long(p);

                        if (mng_info->ticks_per_second != 0)
                          frame_delay/=mng_info->ticks_per_second;

                        else
                          frame_delay=PNG_UINT_31_MAX;

                        if (change_delay == 2)
                          default_frame_delay=frame_delay;

                        p+=(ptrdiff_t) 4;

                        if (logging != MagickFalse)
                          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "    Framing_delay=%.20g",(double) frame_delay);
                      }

                    if (change_timeout && ((p-chunk)+4 <= (ssize_t) length))
                      {
                        frame_timeout=(size_t) image->ticks_per_second*
                          (size_t) mng_get_long(p);

                        if (mng_info->ticks_per_second != 0)
                          frame_timeout/=mng_info->ticks_per_second;

                        else
                          frame_timeout=PNG_UINT_31_MAX;

                        if (change_timeout == 2)
                          default_frame_timeout=frame_timeout;

                        p+=(ptrdiff_t) 4;

                        if (logging != MagickFalse)
                          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "    Framing_timeout=%.20g",(double) frame_timeout);
                      }