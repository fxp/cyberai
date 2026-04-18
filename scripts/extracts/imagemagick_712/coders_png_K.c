// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 43/94



#if (MAGICKCORE_QUANTUM_DEPTH > 16)
#define QM unsigned short
                if (magn_methx != 1 || magn_methy != 1)
                  {
                  /*
                     Scale pixels to unsigned shorts to prevent
                     overflow of intermediate values of interpolations
                  */
                     for (y=0; y < (ssize_t) image->rows; y++)
                     {
                       q=GetAuthenticPixels(image,0,y,image->columns,1,
                          exception);
                       if (q == (Quantum *) NULL)
                         break;
                       for (x=(ssize_t) image->columns-1; x >= 0; x--)
                       {
                          SetPixelRed(image,ScaleQuantumToShort(
                            GetPixelRed(image,q)),q);
                          SetPixelGreen(image,ScaleQuantumToShort(
                            GetPixelGreen(image,q)),q);
                          SetPixelBlue(image,ScaleQuantumToShort(
                            GetPixelBlue(image,q)),q);
                          SetPixelAlpha(image,ScaleQuantumToShort(
                            GetPixelAlpha(image,q)),q);
                          q+=(ptrdiff_t) GetPixelChannels(image);
                       }

                       if (SyncAuthenticPixels(image,exception) == MagickFalse)
                         break;
                     }
                  }
#else
#define QM Quantum
#endif

                if (image->alpha_trait != UndefinedPixelTrait)
                   (void) SetImageBackgroundColor(large_image,exception);

                else
                  {
                    large_image->background_color.alpha=OpaqueAlpha;
                    (void) SetImageBackgroundColor(large_image,exception);

                    if (magn_methx == 4)
                      magn_methx=2;

                    if (magn_methx == 5)
                      magn_methx=3;

                    if (magn_methy == 4)
                      magn_methy=2;

                    if (magn_methy == 5)
                      magn_methy=3;
                  }

                /* magnify the rows into the right side of the large image */

                if (logging != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "    Magnify the rows to %.20g",
                    (double) large_image->rows);
                m=(ssize_t) mng_info->magn_mt;
                yy=0;
                length=(size_t) GetPixelChannels(image)*image->columns;
                next=(Quantum *) AcquireQuantumMemory(length,sizeof(*next));
                prev=(Quantum *) AcquireQuantumMemory(length,sizeof(*prev));

                if ((prev == (Quantum *) NULL) ||
                    (next == (Quantum *) NULL))
                  {
                    if (prev != (Quantum *) NULL)
                      prev=(Quantum *) RelinquishMagickMemory(prev);
                    if (next != (Quantum *) NULL)
                      next=(Quantum *) RelinquishMagickMemory(next);
                    image=DestroyImageList(image);
                    ThrowReaderException(ResourceLimitError,
                      "MemoryAllocationFailed");
                  }

                n=GetAuthenticPixels(image,0,0,image->columns,1,exception);
                (void) memcpy(next,n,length);

                for (y=0; y < (ssize_t) image->rows; y++)
                {
                  if (y == 0)
                    m=(ssize_t) mng_info->magn_mt;

                  else if (magn_methy > 1 && y == (ssize_t) image->rows-2)
                    m=(ssize_t) mng_info->magn_mb;

                  else if (magn_methy <= 1 && y == (ssize_t) image->rows-1)
                    m=(ssize_t) mng_info->magn_mb;

                  else if (magn_methy > 1 && y == (ssize_t) image->rows-1)
                    m=1;

                  else
                    m=(ssize_t) mng_info->magn_my;

                  n=prev;
                  prev=next;
                  next=n;

                  if (y < (ssize_t) image->rows-1)
                    {
                      n=GetAuthenticPixels(image,0,y+1,image->columns,1,
                          exception);
                      (void) memcpy(next,n,length);
                    }

                  for (i=0; i < m; i++, yy++)
                  {
                    Quantum
                      *pixels;

                    assert(yy < (ssize_t) large_image->rows);
                    pixels=prev;
                    n=next;
                    q=GetAuthenticPixels(large_image,0,yy,large_image->columns,
                      1,exception);
                    if (q == (Quantum *) NULL)
                      break;
                    q+=(ptrdiff_t) (large_image->columns-image->columns)*
                      GetPixelChannels(large_image);