// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/constitute.c
// Segment 15/15



  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  assert(exception != (ExceptionInfo *) NULL);
  write_info=CloneImageInfo(image_info);
  *write_info->magick='\0';
  images=GetFirstImageInList(images);
  if (images == (Image *) NULL)
    return(MagickFalse);
  if (filename != (const char *) NULL)
    for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
      (void) CopyMagickString(p->filename,filename,MagickPathExtent);
  (void) CopyMagickString(write_info->filename,images->filename,
    MagickPathExtent);
  sans_exception=AcquireExceptionInfo();
  (void) SetImageInfo(write_info,(unsigned int) GetImageListLength(images),
    sans_exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  if (*write_info->magick == '\0')
    (void) CopyMagickString(write_info->magick,images->magick,MagickPathExtent);
  p=images;
  for ( ; GetNextImageInList(p) != (Image *) NULL; p=GetNextImageInList(p))
  {
    if (p->scene >= GetNextImageInList(p)->scene)
      {
        ssize_t
          i;

        /*
          Generate consistent scene numbers.
        */
        i=(ssize_t) images->scene;
        for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
          p->scene=(size_t) i++;
        break;
      }
  }
  /*
    Write images.
  */
  status=MagickTrue;
  progress_monitor=(MagickProgressMonitor) NULL;
  progress=0;
  number_images=GetImageListLength(images);
  for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    if (number_images != 1)
      progress_monitor=SetImageProgressMonitor(p,(MagickProgressMonitor) NULL,
        p->client_data);
    status&=(MagickStatusType) WriteImage(write_info,p,exception);
    if (number_images != 1)
      (void) SetImageProgressMonitor(p,progress_monitor,p->client_data);
    if (write_info->adjoin != MagickFalse)
      break;
    if (number_images != 1)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(p,WriteImageTag,progress,number_images);
        if (proceed == MagickFalse)
          break;
      }
  }
  write_info=DestroyImageInfo(write_info);
  return(status != 0 ? MagickTrue : MagickFalse);
}
