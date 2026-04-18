// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 42/94



    if (image == (Image *) NULL)
      {
        if (logging != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "exit ReadJNGImage() with error");
        return((Image *) NULL);
      }

    if (image->columns == 0 || image->rows == 0)
      {
        (void) CloseBlob(image);
        return(DestroyImageList(image));
      }

    if (exception->severity > ErrorException)
      {
        (void) CloseBlob(image);
        return(DestroyImageList(image));
      }

    mng_info->image=image;

    if (mng_type)
      {
        MngBox
          crop_box;

        if (((mng_info->magn_methx > 0) && (mng_info->magn_methx <= 5)) &&
            ((mng_info->magn_methy > 0) && (mng_info->magn_methy <= 5)))
          {
            size_t
               magnified_height,
               magnified_width;

            if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "  Processing MNG MAGN chunk");

            if (image->columns == 1)
              mng_info->magn_methx = 1;
            if (image->rows == 1)
              mng_info->magn_methy = 1;
            if (mng_info->magn_methx == 1)
              {
                magnified_width=(size_t) mng_info->magn_ml;

                if (image->columns > 1)
                   magnified_width += mng_info->magn_mr;

                if (image->columns > 2)
                   magnified_width += (size_t)
                      ((image->columns-2)*(mng_info->magn_mx));
              }

            else
              {
                magnified_width=(size_t) image->columns;

                if (image->columns > 1)
                   magnified_width += mng_info->magn_ml-1;

                if (image->columns > 2)
                   magnified_width += mng_info->magn_mr-1;

                if (image->columns > 3)
                   magnified_width += (size_t)
                      ((image->columns-3)*(mng_info->magn_mx-1));
              }

            if (mng_info->magn_methy == 1)
              {
                magnified_height=(size_t) mng_info->magn_mt;

                if (image->rows > 1)
                   magnified_height += mng_info->magn_mb;

                if (image->rows > 2)
                   magnified_height += (size_t)
                      ((image->rows-2)*(mng_info->magn_my));
              }

            else
              {
                magnified_height=(size_t) image->rows;

                if (image->rows > 1)
                   magnified_height += mng_info->magn_mt-1;

                if (image->rows > 2)
                   magnified_height += mng_info->magn_mb-1;

                if (image->rows > 3)
                   magnified_height += (size_t)
                      ((image->rows-3)*(mng_info->magn_my-1));
              }

            if (magnified_height > image->rows ||
                magnified_width > image->columns)
              {
                Image
                  *large_image;

                int
                  yy;

                Quantum
                  *next,
                  *prev;

                png_uint_16
                  magn_methx,
                  magn_methy;

                ssize_t
                  m,
                  y;

                Quantum
                  *n,
                  *q;

                ssize_t
                  x;

                /* Allocate next image structure.  */

                if (logging != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "    Allocate magnified image");

                AcquireNextImage(image_info,image,exception);

                if (GetNextImageInList(image) == (Image *) NULL)
                  return(DestroyImageList(image));

                large_image=SyncNextImageInList(image);

                large_image->columns=magnified_width;
                large_image->rows=magnified_height;

                status=SetImageExtent(image,image->columns,image->rows,exception);
                if (status == MagickFalse)
                  return(DestroyImageList(image));

                magn_methx=mng_info->magn_methx;
                magn_methy=mng_info->magn_methy;