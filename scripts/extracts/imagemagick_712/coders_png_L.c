// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 44/94



                    for (x=(ssize_t) image->columns-1; x >= 0; x--)
                    {
                      /* To do: get color as function of indexes[x] */
                      /*
                      if (image->storage_class == PseudoClass)
                        {
                        }
                      */

                      if (magn_methy <= 1)
                        {
                          /* replicate previous */
                          SetPixelRed(large_image,GetPixelRed(image,pixels),q);
                          SetPixelGreen(large_image,GetPixelGreen(image,
                             pixels),q);
                          SetPixelBlue(large_image,GetPixelBlue(image,
                             pixels),q);
                          SetPixelAlpha(large_image,GetPixelAlpha(image,
                             pixels),q);
                        }

                      else if (magn_methy == 2 || magn_methy == 4)
                        {
                          if (i == 0)
                            {
                              SetPixelRed(large_image,GetPixelRed(image,
                                 pixels),q);
                              SetPixelGreen(large_image,GetPixelGreen(image,
                                 pixels),q);
                              SetPixelBlue(large_image,GetPixelBlue(image,
                                 pixels),q);
                              SetPixelAlpha(large_image,GetPixelAlpha(image,
                                 pixels),q);
                            }

                          else
                            {
                              /* Interpolate */
                              SetPixelRed(large_image,((QM) (((ssize_t)
                                 (2*i*(GetPixelRed(image,n)
                                 -GetPixelRed(image,pixels)+m))/
                                 ((ssize_t) (m*2))
                                 +GetPixelRed(image,pixels)))),q);
                              SetPixelGreen(large_image,((QM) (((ssize_t)
                                 (2*i*(GetPixelGreen(image,n)
                                 -GetPixelGreen(image,pixels)+m))/
                                 ((ssize_t) (m*2))
                                 +GetPixelGreen(image,pixels)))),q);
                              SetPixelBlue(large_image,((QM) (((ssize_t)
                                 (2*i*(GetPixelBlue(image,n)
                                 -GetPixelBlue(image,pixels)+m))/
                                 ((ssize_t) (m*2))
                                 +GetPixelBlue(image,pixels)))),q);

                              if (image->alpha_trait != UndefinedPixelTrait)
                                 SetPixelAlpha(large_image, ((QM) (((ssize_t)
                                    (2*i*(GetPixelAlpha(image,n)
                                    -GetPixelAlpha(image,pixels)+m))
                                    /((ssize_t) (m*2))+
                                   GetPixelAlpha(image,pixels)))),q);
                            }

                          if (magn_methy == 4)
                            {
                              /* Replicate nearest */
                              if (i <= ((m+1) << 1))
                                 SetPixelAlpha(large_image,GetPixelAlpha(image,
                                    pixels),q);
                              else
                                 SetPixelAlpha(large_image,GetPixelAlpha(image,
                                    n),q);
                            }
                        }

                      else /* if (magn_methy == 3 || magn_methy == 5) */
                        {
                          /* Replicate nearest */
                          if (i <= ((m+1) << 1))
                          {
                             SetPixelRed(large_image,GetPixelRed(image,
                                    pixels),q);
                             SetPixelGreen(large_image,GetPixelGreen(image,
                                    pixels),q);
                             SetPixelBlue(large_image,GetPixelBlue(image,
                                    pixels),q);
                             SetPixelAlpha(large_image,GetPixelAlpha(image,
                                    pixels),q);
                          }

                          else
                          {
                             SetPixelRed(large_image,GetPixelRed(image,n),q);
                             SetPixelGreen(large_image,GetPixelGreen(image,n),
                                    q);
                             SetPixelBlue(large_image,GetPixelBlue(image,n),
                                    q);
                             SetPixelAlpha(large_image,GetPixelAlpha(image,n),
                                    q);
                          }