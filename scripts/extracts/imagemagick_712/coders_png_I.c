// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 41/94



                image->delay=0;
                image->columns=mng_info->mng_width;
                image->rows=mng_info->mng_height;
                image->page.width=mng_info->mng_width;
                image->page.height=mng_info->mng_height;
                image->page.x=0;
                image->page.y=0;
                image->background_color=mng_background_color;
                (void) SetImageBackgroundColor(image,exception);
                if (logging != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "  Inserted transparent background layer, W=%.20g, H=%.20g",
                    (double) mng_info->mng_width,(double) mng_info->mng_height);
              }
          }
        /*
          Insert a background layer behind the upcoming image if
          framing_mode is 3, and we haven't already inserted one.
        */
        if (insert_layers && (mng_info->framing_mode == 3) &&
                (subframe_width) && (subframe_height) && (simplicity == 0 ||
                (simplicity & 0x08)))
          {
            if (GetAuthenticPixelQueue(image) != (Quantum *) NULL)
            {
              /*
                Allocate next image structure.
              */
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

            image->delay=0;
            image->columns=subframe_width;
            image->rows=subframe_height;
            image->page.width=subframe_width;
            image->page.height=subframe_height;
            image->page.x=mng_info->clip.left;
            image->page.y=mng_info->clip.top;
            image->background_color=mng_background_color;
            image->alpha_trait=UndefinedPixelTrait;
            (void) SetImageBackgroundColor(image,exception);

            if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "  Insert background layer, L=%.20g, R=%.20g T=%.20g, B=%.20g",
                (double) mng_info->clip.left,(double) mng_info->clip.right,
                (double) mng_info->clip.top,(double) mng_info->clip.bottom);
          }
        first_mng_object=MagickFalse;

        if (GetAuthenticPixelQueue(image) != (Quantum *) NULL)
          {
            /*
              Allocate next image structure.
            */
            AcquireNextImage(image_info,image,exception);

            if (GetNextImageInList(image) == (Image *) NULL)
              return(DestroyImageList(image));

            image=SyncNextImageInList(image);
          }
        mng_info->image=image;
        status=SetImageProgress(image,LoadImagesTag,TellBlob(image),
          GetBlobSize(image));

        if (status == MagickFalse)
          break;

        if (term_chunk_found)
          {
            image->start_loop=MagickTrue;
            term_chunk_found=MagickFalse;
          }

        else
            image->start_loop=MagickFalse;

        if (mng_info->framing_mode == 1 || mng_info->framing_mode == 3)
          {
            image->delay=frame_delay;
            frame_delay=default_frame_delay;
          }

        else
          image->delay=0;

        if (mng_info->framing_mode == 3)
          image->dispose=BackgroundDispose;
        else
          image->dispose=NoneDispose;

        image->page.width=mng_info->mng_width;
        image->page.height=mng_info->mng_height;
        image->page.x=mng_info->x_off[object_id];
        image->page.y=mng_info->y_off[object_id];
        image->iterations=mng_iterations;

        /*
          Seek back to the beginning of the IHDR or JHDR chunk's length field.
        */

        if (logging != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "  Seeking back to beginning of %c%c%c%c chunk",type[0],type[1],
            type[2],type[3]);

        offset=SeekBlob(image,-((ssize_t) length+12),SEEK_CUR);

        if (offset < 0)
          ThrowReaderException(CorruptImageError,"ImproperImageHeader");
      }

    mng_info->image=image;
    mng_info->mng_type=mng_type;
    mng_info->object_id=object_id;

    if (memcmp(type,mng_IHDR,4) == 0)
      image=ReadOnePNGImage(mng_info,image_info,exception);

#if defined(JNG_SUPPORTED)
    else
      image=ReadOneJNGImage(mng_info,image_info,exception);
#endif