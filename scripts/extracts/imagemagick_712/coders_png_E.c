// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 37/94



                    if (change_clipping && ((p-chunk) < (ssize_t) (length-16)))
                      {
                        fb=mng_read_box(previous_fb,(char) p[0],&p[1]);
                        p+=(ptrdiff_t) 16;
                        previous_fb=fb;

                        if (logging != MagickFalse)
                          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "    Frame_clip: L=%.20g R=%.20g T=%.20g B=%.20g",
                            (double) fb.left,(double) fb.right,(double) fb.top,
                            (double) fb.bottom);

                        if (change_clipping == 2)
                          default_fb=fb;
                      }
                  }
              }
            mng_info->clip=fb;
            mng_info->clip=mng_minimum_box(fb,mng_info->frame);

            subframe_width=(size_t) (mng_info->clip.right
               -mng_info->clip.left);

            subframe_height=(size_t) (mng_info->clip.bottom
               -mng_info->clip.top);
            /*
              Insert a background layer behind the frame if framing_mode is 4.
            */
            if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "   subframe_width=%.20g, subframe_height=%.20g",(double)
                subframe_width,(double) subframe_height);

            if (insert_layers && (mng_info->framing_mode == 4) &&
                (subframe_width) && (subframe_height))
              {
                /* Allocate next image structure.  */
                if (GetAuthenticPixelQueue(image) != (Quantum *) NULL)
                  {
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

                image->columns=subframe_width;
                image->rows=subframe_height;
                image->page.width=subframe_width;
                image->page.height=subframe_height;
                image->page.x=mng_info->clip.left;
                image->page.y=mng_info->clip.top;
                image->background_color=mng_background_color;
                image->alpha_trait=UndefinedPixelTrait;
                image->delay=0;
                if (SetImageBackgroundColor(image,exception) == MagickFalse)
                  {
                    chunk=(unsigned char *) RelinquishMagickMemory(chunk);
                    return(DestroyImageList(image));
                  }
                if (logging != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "  Insert backgd layer, L=%.20g, R=%.20g T=%.20g, B=%.20g",
                    (double) mng_info->clip.left,
                    (double) mng_info->clip.right,
                    (double) mng_info->clip.top,
                    (double) mng_info->clip.bottom);
              }
            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }

        if (memcmp(type,mng_CLIP,4) == 0)
          {
            unsigned int
              first_object,
              last_object;

            /*
              Read CLIP.
            */
            if (length > 3)
              {
                first_object=(unsigned int) ((p[0] << 8) | p[1]);
                last_object=(unsigned int) ((p[2] << 8) | p[3]);
                p+=(ptrdiff_t) 4;

                for (i=(int) first_object; i <= (int) last_object; i++)
                {
                  if ((i < 0) || (i >= MNG_MAX_OBJECTS))
                    continue;

                  if (mng_info->exists[i] && !mng_info->frozen[i])
                    {
                      MngBox
                        box;

                      box=mng_info->object_clip[i];
                      if ((p-chunk) < (ssize_t) (length-17))
                        mng_info->object_clip[i]=
                           mng_read_box(box,(char) p[0],&p[1]);
                    }
                }

              }
            chunk=(unsigned char *) RelinquishMagickMemory(chunk);
            continue;
          }

        if (memcmp(type,mng_SAVE,4) == 0)
          {
            for (i=1; i < MNG_MAX_OBJECTS; i++)
              if (mng_info->exists[i])
                 mng_info->frozen[i]=MagickTrue;

            chunk=(unsigned char *) RelinquishMagickMemory(chunk);

            continue;
          }