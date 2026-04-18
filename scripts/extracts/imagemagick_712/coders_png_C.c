// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 35/94



            if (length > 2)
              mng_info->invisible[object_id]=p[2];

            /*
              Extract object offset info.
            */
            if (length > 11)
              {
                mng_info->x_off[object_id]=(ssize_t) mng_get_long(&p[4]);
                mng_info->y_off[object_id]=(ssize_t) mng_get_long(&p[8]);
                if (logging != MagickFalse)
                  {
                    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                      "  x_off[%d]: %.20g,  y_off[%d]: %.20g",
                      object_id,(double) mng_info->x_off[object_id],
                      object_id,(double) mng_info->y_off[object_id]);
                  }
              }

            /*
              Extract object clipping info.
            */
            if (length > 27)
              mng_info->object_clip[object_id]=mng_read_box(mng_info->frame,0,
                &p[12]);

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }
        if (memcmp(type,mng_bKGD,4) == 0)
          {
            mng_info->have_global_bkgd=MagickFalse;

            if (length > 5)
              {
                mng_info->mng_global_bkgd.red=ScaleShortToQuantum(
                  (unsigned short) (((unsigned int) p[0] << 8) | p[1]));
                mng_info->mng_global_bkgd.green=ScaleShortToQuantum(
                  (unsigned short) (((unsigned int) p[2] << 8) | p[3]));
                mng_info->mng_global_bkgd.blue=ScaleShortToQuantum(
                  (unsigned short) (((unsigned int) p[4] << 8) | p[5]));
                mng_info->have_global_bkgd=MagickTrue;
              }

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }
        if (memcmp(type,mng_BACK,4) == 0)
          {
            if (length > 6)
              mandatory_back=p[6];

            else
              mandatory_back=0;

            if (mandatory_back && length > 5)
              {
                mng_background_color.red=ScaleShortToQuantum((unsigned short)
                  (((unsigned int) p[0] << 8) | p[1]));
                mng_background_color.green=ScaleShortToQuantum((unsigned short)
                  (((unsigned int) p[2] << 8) | p[3]));
                mng_background_color.blue=ScaleShortToQuantum((unsigned short)
                  (((unsigned int) p[4] << 8) | p[5]));
                mng_background_color.alpha=OpaqueAlpha;
              }
            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }

        if (memcmp(type,mng_PLTE,4) == 0)
          {
            /* Read global PLTE.  */

            if (length && (length < 769))
              {
                /* Read global PLTE.  */

                if (mng_info->global_plte == (png_colorp) NULL)
                  mng_info->global_plte=(png_colorp) AcquireQuantumMemory(256,
                    sizeof(*mng_info->global_plte));

                if (mng_info->global_plte == (png_colorp) NULL)
                  {
                    mng_info->global_plte_length=0;
                    chunk=(unsigned char *) RelinquishMagickMemory(chunk);
                    ThrowReaderException(ResourceLimitError,
                      "MemoryAllocationFailed");
                  }

                for (i=0; i < (ssize_t) (length/3); i++)
                {
                  mng_info->global_plte[i].red=p[3*i];
                  mng_info->global_plte[i].green=p[3*i+1];
                  mng_info->global_plte[i].blue=p[3*i+2];
                }

                mng_info->global_plte_length=(unsigned int) (length/3);
              }
            else
              mng_info->global_plte_length=0;

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }

        if (memcmp(type,mng_tRNS,4) == 0)
          {
            /* read global tRNS */

            if (length > 0 && length < 257)
              for (i=0; i < (ssize_t) length; i++)
                mng_info->global_trns[i]=p[i];
            mng_info->global_trns_length=(unsigned int) length;
            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }
        if (memcmp(type,mng_gAMA,4) == 0)
          {
            if (length == 4)
              {
                ssize_t
                  igamma;

                igamma=mng_get_long(p);
                mng_info->global_gamma=((float) igamma)*0.00001f;
                mng_info->have_global_gama=MagickTrue;
              }

            else
              mng_info->have_global_gama=MagickFalse;

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }

        if (memcmp(type,mng_cHRM,4) == 0)
          {
            /* Read global cHRM */