// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 47/94



        /*
          Crop_box is with respect to the upper left corner of the MNG.
        */
        crop_box.left=mng_info->image_box.left+(long) mng_info->x_off[object_id];
        crop_box.right=mng_info->image_box.right+(long) mng_info->x_off[object_id];
        crop_box.top=mng_info->image_box.top+(long) mng_info->y_off[object_id];
        crop_box.bottom=mng_info->image_box.bottom+(long) mng_info->y_off[object_id];
        crop_box=mng_minimum_box(crop_box,mng_info->clip);
        crop_box=mng_minimum_box(crop_box,mng_info->frame);
        crop_box=mng_minimum_box(crop_box,mng_info->object_clip[object_id]);
        if ((crop_box.left != (mng_info->image_box.left
            +mng_info->x_off[object_id])) ||
            (crop_box.right != (mng_info->image_box.right
            +mng_info->x_off[object_id])) ||
            (crop_box.top != (mng_info->image_box.top
            +mng_info->y_off[object_id])) ||
            (crop_box.bottom != (mng_info->image_box.bottom
            +mng_info->y_off[object_id])))
          {
            if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "  Crop the PNG image");

            if ((crop_box.left < crop_box.right) &&
                (crop_box.top < crop_box.bottom))
              {
                Image
                  *im;

                RectangleInfo
                  crop_info;

                /*
                  Crop_info is with respect to the upper left corner of
                  the image.
                */
                crop_info.x=(crop_box.left-mng_info->x_off[object_id]);
                crop_info.y=(crop_box.top-mng_info->y_off[object_id]);
                crop_info.width=(size_t) (crop_box.right-crop_box.left);
                crop_info.height=(size_t) (crop_box.bottom-crop_box.top);
                image->page.width=image->columns;
                image->page.height=image->rows;
                image->page.x=0;
                image->page.y=0;
                im=CropImage(image,&crop_info,exception);

                if (im != (Image *) NULL)
                  {
                    image->columns=im->columns;
                    image->rows=im->rows;
                    im=DestroyImage(im);
                    image->page.width=image->columns;
                    image->page.height=image->rows;
                    image->page.x=crop_box.left;
                    image->page.y=crop_box.top;
                  }
              }

            else
              {
                /*
                  No pixels in crop area.  The MNG spec still requires
                  a layer, though, so make a single transparent pixel in
                  the top left corner.
                */
                image->columns=1;
                image->rows=1;
                image->colors=2;
                (void) SetImageBackgroundColor(image,exception);
                image->page.width=1;
                image->page.height=1;
                image->page.x=0;
                image->page.y=0;
              }
          }
      }

#if (MAGICKCORE_QUANTUM_DEPTH > 16)
      /* PNG does not handle depths greater than 16 so reduce it even
       * if lossy.
       */
      if (image->depth > 16)
         image->depth=16;
#endif

#if (MAGICKCORE_QUANTUM_DEPTH > 8)
      if (image->depth > 8)
        {
          /* To do: fill low byte properly */
          image->depth=16;
        }

#endif

      if (image_info->number_scenes != 0)
        {
          if (mng_info->scenes_found >
             (ssize_t) (image_info->scene+image_info->number_scenes))
            break;
        }

      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "  Finished reading image datastream.");

  } while (LocaleCompare(image_info->magick,"MNG") == 0);

  (void) CloseBlob(image);

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "  Finished reading all image datastreams.");

  if (insert_layers && !mng_info->image_found && (mng_info->mng_width) &&
       (mng_info->mng_height))
    {
      /*
        Insert a background layer if nothing else was found.
      */
      if (logging != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "  No images found.  Inserting a background layer.");

      if (GetAuthenticPixelQueue(image) != (Quantum *) NULL)
        {
          /*
            Allocate next image structure.
          */
          AcquireNextImage(image_info,image,exception);
          if (GetNextImageInList(image) == (Image *) NULL)
            {
              if (logging != MagickFalse)
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Allocation failed, returning NULL.");