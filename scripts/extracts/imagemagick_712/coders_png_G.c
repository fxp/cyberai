// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 39/94



                            if (offset < 0)
                              {
                                chunk=(unsigned char *) RelinquishMagickMemory(
                                  chunk);
                                ThrowReaderException(CorruptImageError,
                                  "ImproperImageHeader");
                              }
                          }

                        else
                          {
                            short
                              last_level;

                            /*
                              Finished loop.
                            */
                            mng_info->loop_active[loop_level]=0;
                            last_level=(-1);
                            for (i=0; i < loop_level; i++)
                              if (mng_info->loop_active[i] == 1)
                                last_level=(short) i;
                            loop_level=last_level;
                          }
                      }
                  }
              }

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }

        if (memcmp(type,mng_CLON,4) == 0)
          {
            if (mng_info->clon_warning == 0)
              (void) ThrowMagickException(exception,GetMagickModule(),
                CoderError,"CLON is not implemented yet","`%s'",
                image->filename);

            mng_info->clon_warning++;
          }

        if (memcmp(type,mng_MAGN,4) == 0)
          {
            png_uint_16
              magn_first,
              magn_last,
              magn_mb,
              magn_ml,
              magn_mr,
              magn_mt,
              magn_mx,
              magn_my,
              magn_methx,
              magn_methy;

            if (length > 1)
              magn_first=(p[0] << 8) | p[1];

            else
              magn_first=0;

            if (length > 3)
              magn_last=(p[2] << 8) | p[3];

            else
              magn_last=magn_first;
            if (magn_first || magn_last)
              if (mng_info->magn_warning == 0)
                {
                  (void) ThrowMagickException(exception,
                     GetMagickModule(),CoderError,
                     "MAGN is not implemented yet for nonzero objects",
                     "`%s'",image->filename);

                   mng_info->magn_warning++;
                }
            if (length > 4)
              magn_methx=p[4];

            else
              magn_methx=0;

            if (length > 6)
              magn_mx=(p[5] << 8) | p[6];

            else
              magn_mx=1;

            if (magn_mx == 0)
              magn_mx=1;

            if (length > 8)
              magn_my=(p[7] << 8) | p[8];

            else
              magn_my=magn_mx;

            if (magn_my == 0)
              magn_my=1;

            if (length > 10)
              magn_ml=(p[9] << 8) | p[10];

            else
              magn_ml=magn_mx;

            if (magn_ml == 0)
              magn_ml=1;

            if (length > 12)
              magn_mr=(p[11] << 8) | p[12];

            else
              magn_mr=magn_mx;

            if (magn_mr == 0)
              magn_mr=1;

            if (length > 14)
              magn_mt=(p[13] << 8) | p[14];

            else
              magn_mt=magn_my;

            if (magn_mt == 0)
              magn_mt=1;

            if (length > 16)
              magn_mb=(p[15] << 8) | p[16];

            else
              magn_mb=magn_my;

            if (magn_mb == 0)
              magn_mb=1;

            if (length > 17)
              magn_methy=p[17];

            else
              magn_methy=magn_methx;


            if (magn_methx > 5 || magn_methy > 5)
              if (mng_info->magn_warning == 0)
                {
                  (void) ThrowMagickException(exception,
                     GetMagickModule(),CoderError,
                     "Unknown MAGN method in MNG datastream","`%s'",
                     image->filename);

                   mng_info->magn_warning++;
                }
            if (magn_first == 0 || magn_last == 0)
              {
                /* Save the magnification factors for object 0 */
                mng_info->magn_mb=magn_mb;
                mng_info->magn_ml=magn_ml;
                mng_info->magn_mr=magn_mr;
                mng_info->magn_mt=magn_mt;
                mng_info->magn_mx=magn_mx;
                mng_info->magn_my=magn_my;
                mng_info->magn_methx=magn_methx;
                mng_info->magn_methy=magn_methy;
              }
          }

        if (memcmp(type,mng_PAST,4) == 0)
          {
            if (mng_info->past_warning == 0)
              (void) ThrowMagickException(exception,GetMagickModule(),
                CoderError,"PAST is not implemented yet","`%s'",
                image->filename);

            mng_info->past_warning++;
          }