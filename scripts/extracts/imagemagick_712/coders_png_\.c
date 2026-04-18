// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 28/94



                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "    jng_image_interlace_method:  %3d"
                  "    jng_alpha_sample_depth:      %3d",
                  jng_image_interlace_method,
                  jng_alpha_sample_depth);

                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "    jng_alpha_compression_method:%3d\n"
                  "    jng_alpha_filter_method:     %3d\n"
                  "    jng_alpha_interlace_method:  %3d",
                  jng_alpha_compression_method,
                  jng_alpha_filter_method,
                  jng_alpha_interlace_method);
              }
          }

        chunk=(unsigned char *) RelinquishMagickMemory(chunk);

        if ((jng_width > 65535) || (jng_height > 65535) ||
            (MagickSizeType) jng_width > GetMagickResourceLimit(WidthResource) ||
            (MagickSizeType) jng_height > GetMagickResourceLimit(HeightResource))
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
               "    JNG width or height too large: (%lu x %lu)",
                (long) jng_width, (long) jng_height);
            DestroyJNG(chunk,&color_image,&color_image_info,
              &alpha_image,&alpha_image_info);
            ThrowReaderException(CorruptImageError,"ImproperImageHeader");
          }

        continue;
      }


    if ((reading_idat == MagickFalse) && (read_JSEP == MagickFalse) &&
        ((memcmp(type,mng_JDAT,4) == 0) || (memcmp(type,mng_JdAA,4) == 0) ||
         (memcmp(type,mng_IDAT,4) == 0) || (memcmp(type,mng_JDAA,4) == 0)))
      {
        /*
           o create color_image
           o open color_blob, attached to color_image
           o if (color type has alpha)
               open alpha_blob, attached to alpha_image
        */

        color_image_info=(ImageInfo *)AcquireMagickMemory(sizeof(ImageInfo));

        if (color_image_info == (ImageInfo *) NULL)
        {
          DestroyJNG(chunk,&color_image,&color_image_info,
              &alpha_image,&alpha_image_info);
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        }

        GetImageInfo(color_image_info);
        color_image=AcquireImage(color_image_info,exception);

        if (color_image == (Image *) NULL)
        {
          DestroyJNG(chunk,&color_image,&color_image_info,
              &alpha_image,&alpha_image_info);
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        }

        if (logging != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "    Creating color_blob.");

        (void) AcquireUniqueFilename(color_image->filename);
        status=OpenBlob(color_image_info,color_image,WriteBinaryBlobMode,
          exception);

        if (status == MagickFalse)
          {
            DestroyJNG(chunk,&color_image,&color_image_info,
              &alpha_image,&alpha_image_info);
            return(DestroyImageList(image));
          }

        if ((image_info->ping == MagickFalse) && (jng_color_type >= 12))
          {
            if ((jng_alpha_compression_method != 0) &&
                (jng_alpha_compression_method != 8))
              {
                DestroyJNG(chunk,&color_image,&color_image_info,&alpha_image,
                  &alpha_image_info);
                ThrowReaderException(CorruptImageError,"ImproperImageHeader");
              }
            alpha_image_info=(ImageInfo *)
              AcquireMagickMemory(sizeof(ImageInfo));

            if (alpha_image_info == (ImageInfo *) NULL)
              {
                DestroyJNG(chunk,&color_image,&color_image_info,
                  &alpha_image,&alpha_image_info);
                ThrowReaderException(ResourceLimitError,
                  "MemoryAllocationFailed");
              }

            GetImageInfo(alpha_image_info);
            alpha_image=AcquireImage(alpha_image_info,exception);

            if (alpha_image == (Image *) NULL)
              {
                DestroyJNG(chunk,&color_image,&color_image_info,
                  &alpha_image,&alpha_image_info);
                ThrowReaderException(ResourceLimitError,
                  "MemoryAllocationFailed");
              }

            if (logging != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "    Creating alpha_blob.");

            (void) AcquireUniqueFilename(alpha_image->filename);
            status=OpenBlob(alpha_image_info,alpha_image,WriteBinaryBlobMode,
              exception);

            if (status == MagickFalse)
              {
                DestroyJNG(chunk,&color_image,&color_image_info,
                  &alpha_image,&alpha_image_info);
                return(DestroyImageList(image));
              }

            if (jng_alpha_compression_method == 0)
              {
                unsigned char
                  data[18];