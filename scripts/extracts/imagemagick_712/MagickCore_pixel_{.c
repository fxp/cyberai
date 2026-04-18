// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/pixel.c
// Segment 59/59



  /*
    Sort image pixels.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns-1; x++)
    {
      MagickRealType
        current,
        previous;

      ssize_t
        j;

      previous=GetPixelIntensity(image,q);
      for (j=0; j < ((ssize_t) image->columns-x-1); j++)
      {
        current=GetPixelIntensity(image,q+(j+1)*(ssize_t)
          GetPixelChannels(image));
        if (previous > current)
          {
            Quantum
              pixel[MaxPixelChannels];

            /*
              Swap adjacent pixels.
            */
            (void) memcpy(pixel,q+j*(ssize_t) GetPixelChannels(image),
              GetPixelChannels(image)*sizeof(Quantum));
            (void) memcpy(q+j*(ssize_t) GetPixelChannels(image),q+(j+1)*
              (ssize_t) GetPixelChannels(image),GetPixelChannels(image)*
              sizeof(Quantum));
            (void) memcpy(q+(j+1)*(ssize_t) GetPixelChannels(image),pixel,
              GetPixelChannels(image)*sizeof(Quantum));
          }
        else
          previous=current;
      }
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,SolarizeImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}
