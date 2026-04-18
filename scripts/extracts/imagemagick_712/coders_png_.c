// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 89/94



        if (p->colors)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "      Number of colors: %.20g",(double) p->colors);

        else
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "      Number of colors: unspecified");

        if (mng_info->adjoin == MagickFalse)
          break;
      }
    }

  use_global_plte=MagickFalse;
  all_images_are_gray=MagickFalse;
  need_local_plte=MagickTrue;
  mng_info->need_defi=MagickFalse;
  need_matte=MagickFalse;
  mng_info->framing_mode=1;
  mng_info->old_framing_mode=1;

  if (write_mng != MagickFalse)
    {
      unsigned int
        need_geom;

      unsigned short
        red,
        green,
        blue;

      const char *
        option;

      if (image_info->page != (char *) NULL)
        {
          /*
            Determine image bounding box.
          */
          SetGeometry(image,&mng_info->page);
          (void) ParseMetaGeometry(image_info->page,&mng_info->page.x,
            &mng_info->page.y,&mng_info->page.width,&mng_info->page.height);
        }
      mng_info->page=image->page;
      need_geom=MagickTrue;
      if (mng_info->page.width || mng_info->page.height)
         need_geom=MagickFalse;
      /*
        Check all the scenes.
      */
      initial_delay=image->delay;
      need_iterations=MagickFalse;
      mng_info->equal_chrms=image->chromaticity.red_primary.x != 0.0 ?
        MagickTrue : MagickFalse;
      mng_info->equal_physs=MagickTrue,
      mng_info->equal_gammas=MagickTrue;
      mng_info->equal_srgbs=MagickTrue;
      mng_info->equal_backgrounds=MagickTrue;
      image_count=0;
      all_images_are_gray=MagickTrue;
      mng_info->equal_palettes=MagickFalse;
      need_local_plte=MagickFalse;
      for (next_image=image; next_image != (Image *) NULL; )
      {
        if (need_geom)
          {
            if ((next_image->columns+(size_t) next_image->page.x) > mng_info->page.width)
              mng_info->page.width=next_image->columns+(size_t) next_image->page.x;

            if ((next_image->rows+(size_t) next_image->page.y) > mng_info->page.height)
              mng_info->page.height=next_image->rows+(size_t) next_image->page.y;
          }

        if (next_image->page.x || next_image->page.y)
          mng_info->need_defi=MagickTrue;

        if (next_image->alpha_trait != UndefinedPixelTrait)
          need_matte=MagickTrue;

        if ((int) next_image->dispose >= BackgroundDispose)
          if ((next_image->alpha_trait != UndefinedPixelTrait) ||
               next_image->page.x || next_image->page.y ||
              ((next_image->columns < mng_info->page.width) &&
               (next_image->rows < mng_info->page.height)))
            mng_info->need_fram=MagickTrue;

        if (next_image->iterations)
          need_iterations=MagickTrue;

        final_delay=next_image->delay;

        if (final_delay != initial_delay || (ssize_t) final_delay > 
           next_image->ticks_per_second)
          mng_info->need_fram=MagickTrue;

        /*
          check for global palette possibility.
        */
        if (image->alpha_trait != UndefinedPixelTrait)
           need_local_plte=MagickTrue;

        if (need_local_plte == 0)
          {
            if (IdentifyImageCoderGray(image,exception) == MagickFalse)
              all_images_are_gray=MagickFalse;
            mng_info->equal_palettes=PalettesAreEqual(image,next_image);
            if (use_global_plte == 0)
              use_global_plte=(int) mng_info->equal_palettes;
            need_local_plte=!mng_info->equal_palettes;
          }
        if (GetNextImageInList(next_image) != (Image *) NULL)
          {
            if (next_image->background_color.red !=
                next_image->next->background_color.red ||
                next_image->background_color.green !=
                next_image->next->background_color.green ||
                next_image->background_color.blue !=
                next_image->next->background_color.blue)
              mng_info->equal_backgrounds=MagickFalse;

            if (next_image->gamma != next_image->next->gamma)
              mng_info->equal_gammas=MagickFalse;

            if (next_image->rendering_intent !=
                next_image->next->rendering_intent)
              mng_info->equal_srgbs=MagickFalse;

            if ((next_image->units != next_image->next->units) ||
                (next_image->resolution.x != next_image->next->resolution.x) ||
                (next_image->resolution.y != next_image->next->resolution.y))
              mng_info->equal_physs=MagickFalse;