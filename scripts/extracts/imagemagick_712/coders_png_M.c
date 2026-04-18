// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 45/94



                          if (magn_methy == 5)
                            {
                              SetPixelAlpha(large_image,(QM) (((ssize_t) (2*i*
                                 (GetPixelAlpha(image,n)
                                 -GetPixelAlpha(image,pixels))
                                 +m))/((ssize_t) (m*2))
                                 +GetPixelAlpha(image,pixels)),q);
                            }
                        }
                      n+=(ptrdiff_t) GetPixelChannels(image);
                      q+=(ptrdiff_t) GetPixelChannels(large_image);
                      pixels+=(ptrdiff_t) GetPixelChannels(image);
                    } /* x */

                    if (SyncAuthenticPixels(large_image,exception) == 0)
                      break;

                  } /* i */
                } /* y */

                prev=(Quantum *) RelinquishMagickMemory(prev);
                next=(Quantum *) RelinquishMagickMemory(next);

                length=image->columns;

                if (logging != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "    Delete original image");

                DeleteImageFromList(&image);

                image=large_image;

                mng_info->image=image;

                /* magnify the columns */
                if (logging != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "    Magnify the columns to %.20g",
                    (double) image->columns);

                for (y=0; y < (ssize_t) image->rows; y++)
                {
                  Quantum
                    *pixels;

                  q=GetAuthenticPixels(image,0,y,image->columns,1,exception);
                  if (q == (Quantum *) NULL)
                    break;
                  pixels=q+(image->columns-length)*GetPixelChannels(image);
                  n=pixels+GetPixelChannels(image);

                  for (x=(ssize_t) (image->columns-length);
                    x < (ssize_t) image->columns; x++)
                  {
                    /* To do: Rewrite using Get/Set***PixelChannel() */

                    if (x == (ssize_t) (image->columns-length))
                      m=(ssize_t) mng_info->magn_ml;

                    else if (magn_methx > 1 && x == (ssize_t) image->columns-2)
                      m=(ssize_t) mng_info->magn_mr;

                    else if (magn_methx <= 1 &&
                        x == (ssize_t) image->columns-1)
                      m=(ssize_t) mng_info->magn_mr;

                    else if (magn_methx > 1 && x == (ssize_t) image->columns-1)
                      m=1;

                    else
                      m=(ssize_t) mng_info->magn_mx;

                    for (i=0; i < m; i++)
                    {
                      if (magn_methx <= 1)
                        {
                          /* replicate previous */
                          SetPixelRed(image,GetPixelRed(image,pixels),q);
                          SetPixelGreen(image,GetPixelGreen(image,pixels),q);
                          SetPixelBlue(image,GetPixelBlue(image,pixels),q);
                          SetPixelAlpha(image,GetPixelAlpha(image,pixels),q);
                        }

                      else if (magn_methx == 2 || magn_methx == 4)
                        {
                          if (i == 0)
                          {
                            SetPixelRed(image,GetPixelRed(image,pixels),q);
                            SetPixelGreen(image,GetPixelGreen(image,pixels),q);
                            SetPixelBlue(image,GetPixelBlue(image,pixels),q);
                            SetPixelAlpha(image,GetPixelAlpha(image,pixels),q);
                          }

                          /* To do: Rewrite using Get/Set***PixelChannel() */
                          else
                            {
                              /* Interpolate */
                              SetPixelRed(image,(QM) ((2*i*(
                                 GetPixelRed(image,n)
                                 -GetPixelRed(image,pixels))+m)
                                 /((ssize_t) (m*2))+
                                 GetPixelRed(image,pixels)),q);

                              SetPixelGreen(image,(QM) ((2*i*(
                                 GetPixelGreen(image,n)
                                 -GetPixelGreen(image,pixels))+m)
                                 /((ssize_t) (m*2))+
                                 GetPixelGreen(image,pixels)),q);