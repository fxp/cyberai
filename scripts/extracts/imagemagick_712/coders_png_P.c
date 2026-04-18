// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 48/94



              return(DestroyImageList(image));;
            }
          image=SyncNextImageInList(image);
        }
      image->columns=mng_info->mng_width;
      image->rows=mng_info->mng_height;
      image->page.width=mng_info->mng_width;
      image->page.height=mng_info->mng_height;
      image->page.x=0;
      image->page.y=0;
      image->background_color=mng_background_color;
      image->alpha_trait=UndefinedPixelTrait;

      if (image_info->ping == MagickFalse)
        (void) SetImageBackgroundColor(image,exception);

      mng_info->image_found++;
    }
  image->iterations=mng_iterations;

  if (mng_iterations == 1)
    image->start_loop=MagickTrue;

  while (GetPreviousImageInList(image) != (Image *) NULL)
  {
    image_count++;
    if (image_count > 10*mng_info->image_found)
      {
        if (logging != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  No beginning");

        (void) ThrowMagickException(exception,GetMagickModule(),
          CoderError,"Linked list is corrupted, beginning of list not found",
          "`%s'",image_info->filename);

        return(DestroyImageList(image));
      }

    image=GetPreviousImageInList(image);

    if (GetNextImageInList(image) == (Image *) NULL)
      {
        if (logging != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  Corrupt list");

        (void) ThrowMagickException(exception,GetMagickModule(),
          CoderError,"Linked list is corrupted; next_image is NULL","`%s'",
          image_info->filename);
      }
  }

  if (mng_info->ticks_per_second && mng_info->image_found > 1 &&
             GetNextImageInList(image) ==
     (Image *) NULL)
    {
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "  First image null");

      (void) ThrowMagickException(exception,GetMagickModule(),
        CoderError,"image->next for first image is NULL but shouldn't be.",
        "`%s'",image_info->filename);
    }

  if (mng_info->image_found == 0)
    {
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "  No visible images found.");

      (void) ThrowMagickException(exception,GetMagickModule(),
        CoderError,"No visible images in file","`%s'",image_info->filename);

      return(DestroyImageList(image));
    }

  if (mng_info->ticks_per_second)
    final_delay=(size_t) MagickMax(image->ticks_per_second,1L)*
            final_delay/mng_info->ticks_per_second;

  else
    image->start_loop=MagickTrue;

  /* Find final nonzero image delay */
  final_image_delay=0;

  while (GetNextImageInList(image) != (Image *) NULL)
    {
      if (image->delay)
        final_image_delay=image->delay;

      image=GetNextImageInList(image);
    }

  if (final_delay < final_image_delay)
    final_delay=final_image_delay;

  image->delay=final_delay;

  if (logging != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  image->delay=%.20g, final_delay=%.20g",(double) image->delay,
        (double) final_delay);

  if (logging != MagickFalse)
    {
      int
        scene;

      scene=0;
      image=GetFirstImageInList(image);

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  Before coalesce:");

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    scene 0 delay=%.20g",(double) image->delay);

      while (GetNextImageInList(image) != (Image *) NULL)
      {
        image=GetNextImageInList(image);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "    scene %.20g delay=%.20g",(double) scene++,
          (double) image->delay);
      }
    }

  image=GetFirstImageInList(image);
  if (insert_layers && image->next)
    {
      Image
        *next_image,
        *next;

      size_t
        scene;

      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "  Coalesce Images");

      scene=image->scene;
      next_image=CoalesceImages(image,exception);
      image=DestroyImageList(image);
      if (next_image == (Image *) NULL)
        return((Image *) NULL);
      image=next_image;

      for (next=image; next != (Image *) NULL; next=next_image)
      {
         next->page.width=mng_info->mng_width;
         next->page.height=mng_info->mng_height;
         next->page.x=0;
         next->page.y=0;
         next->scene=scene++;
         next_image=GetNextImageInList(next);

         if (next_image == (Image *) NULL)
           break;

         if (next->delay == 0)
           {
             scene--;
             next_image->previous=GetPreviousImageInList(next);
             if (GetPreviousImageInList(next) == (Image *) NULL)
               image=next_image;
             else
               next->previous->next=next_image;
             next=DestroyImage(next);
           }
      }
    }