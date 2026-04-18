// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 46/94



                              SetPixelBlue(image,(QM) ((2*i*(
                                 GetPixelBlue(image,n)
                                 -GetPixelBlue(image,pixels))+m)
                                 /((ssize_t) (m*2))+
                                 GetPixelBlue(image,pixels)),q);
                              if (image->alpha_trait != UndefinedPixelTrait)
                                 SetPixelAlpha(image,(QM) ((2*i*(
                                   GetPixelAlpha(image,n)
                                   -GetPixelAlpha(image,pixels))+m)
                                   /((ssize_t) (m*2))+
                                   GetPixelAlpha(image,pixels)),q);
                            }

                          if (magn_methx == 4)
                            {
                              /* Replicate nearest */
                              if (i <= ((m+1) << 1))
                              {
                                 SetPixelAlpha(image,
                                   GetPixelAlpha(image,pixels)+0,q);
                              }
                              else
                              {
                                 SetPixelAlpha(image,
                                   GetPixelAlpha(image,n)+0,q);
                              }
                            }
                        }

                      else /* if (magn_methx == 3 || magn_methx == 5) */
                        {
                          /* Replicate nearest */
                          if (i <= ((m+1) << 1))
                          {
                             SetPixelRed(image,GetPixelRed(image,pixels),q);
                             SetPixelGreen(image,GetPixelGreen(image,
                                 pixels),q);
                             SetPixelBlue(image,GetPixelBlue(image,pixels),q);
                             SetPixelAlpha(image,GetPixelAlpha(image,
                                 pixels),q);
                          }

                          else
                          {
                             SetPixelRed(image,GetPixelRed(image,n),q);
                             SetPixelGreen(image,GetPixelGreen(image,n),q);
                             SetPixelBlue(image,GetPixelBlue(image,n),q);
                             SetPixelAlpha(image,GetPixelAlpha(image,n),q);
                          }

                          if (magn_methx == 5)
                            {
                              /* Interpolate */
                              SetPixelAlpha(image,
                                 (QM) ((2*i*( GetPixelAlpha(image,n)
                                 -GetPixelAlpha(image,pixels))+m)/
                                 ((ssize_t) (m*2))
                                 +GetPixelAlpha(image,pixels)),q);
                            }
                        }
                      q+=(ptrdiff_t) GetPixelChannels(image);
                    }
                    n+=(ptrdiff_t) GetPixelChannels(image);
                  }

                  if (SyncAuthenticPixels(image,exception) == MagickFalse)
                    break;
                }
#if (MAGICKCORE_QUANTUM_DEPTH > 16)
              if (magn_methx != 1 || magn_methy != 1)
                {
                /*
                   Rescale pixels to Quantum
                */
                   for (y=0; y < (ssize_t) image->rows; y++)
                   {
                     q=GetAuthenticPixels(image,0,y,image->columns,1,
                       exception);
                     if (q == (Quantum *) NULL)
                       break;

                     for (x=(ssize_t) image->columns-1; x >= 0; x--)
                     {
                        SetPixelRed(image,ScaleShortToQuantum(
                          (unsigned short) GetPixelRed(image,q)),q);
                        SetPixelGreen(image,ScaleShortToQuantum(
                          (unsigned short) GetPixelGreen(image,q)),q);
                        SetPixelBlue(image,ScaleShortToQuantum(
                          (unsigned short) GetPixelBlue(image,q)),q);
                        SetPixelAlpha(image,ScaleShortToQuantum(
                          (unsigned short) GetPixelAlpha(image,q)),q);
                        q+=(ptrdiff_t) GetPixelChannels(image);
                     }

                     if (SyncAuthenticPixels(image,exception) == MagickFalse)
                       break;
                   }
                }
#endif
                if (logging != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "  Finished MAGN processing");
              }
          }