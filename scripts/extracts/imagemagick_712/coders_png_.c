// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 63/94



           else
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "   Cheap transparency is possible.");
         }
     }
  else
    ping_have_cheap_transparency = MagickFalse;

  image_depth=image->depth;

  quantum_info = (QuantumInfo *) NULL;
  number_colors=0;
  image_matte=image->alpha_trait !=
        UndefinedPixelTrait ? MagickTrue : MagickFalse;

  if (mng_info->colortype < 5)
    mng_info->is_palette=(image->storage_class == PseudoClass) &&
      (image_colors <= 256) && (image->colormap != NULL) ? MagickTrue :
      MagickFalse;
  else
    mng_info->is_palette=MagickFalse;

  if (((mng_info->colortype == 4) || (mng_info->write_png8 != MagickFalse)) &&
     ((image_colors == 0) || (image->colormap == NULL)))
    {
      image_info=DestroyImageInfo(image_info);
      image=DestroyImage(image);
      (void) ThrowMagickException(exception,GetMagickModule(),CoderError,
          "Cannot write PNG8 or color-type 3; colormap is NULL",
          "`%s'",IMimage->filename);
      return(MagickFalse);
    }

  /*
    Allocate the PNG structures
  */
#ifdef PNG_USER_MEM_SUPPORTED
 error_info.image=image;
 error_info.exception=exception;
  ping=png_create_write_struct_2(PNG_LIBPNG_VER_STRING,&error_info,
    MagickPNGErrorHandler,MagickPNGWarningHandler,(void *) NULL,
    (png_malloc_ptr) Magick_png_malloc,(png_free_ptr) Magick_png_free);

#else
  ping=png_create_write_struct(PNG_LIBPNG_VER_STRING,&error_info,
    MagickPNGErrorHandler,MagickPNGWarningHandler);

#endif
  if (ping == (png_struct *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");

  ping_info=png_create_info_struct(ping);

  if (ping_info == (png_info *) NULL)
    {
      png_destroy_write_struct(&ping,(png_info **) NULL);
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    }

  png_set_write_fn(ping,image,png_put_data,png_flush_data);
  pixel_info=(MemoryInfo *) NULL;

  if (setjmp(png_jmpbuf(ping)))
    {
      /*
        PNG write failed.
      */
      png_destroy_write_struct(&ping,&ping_info);
#ifdef IMPNG_SETJMP_NOT_THREAD_SAFE
      UnlockSemaphoreInfo(ping_semaphore);
#endif

      if (pixel_info != (MemoryInfo *) NULL)
        pixel_info=RelinquishVirtualMemory(pixel_info);

      if (quantum_info != (QuantumInfo *) NULL)
        quantum_info=DestroyQuantumInfo(quantum_info);

      if (ping_have_blob != MagickFalse)
          (void) CloseBlob(image);
      image_info=DestroyImageInfo(image_info);
      image=DestroyImage(image);
      return(MagickFalse);
    }

  /* {  For navigation to end of SETJMP-protected block.  Within this
   *    block, use png_error() instead of Throwing an Exception, to ensure
   *    that libpng is able to clean up, and that the semaphore is unlocked.
   */

#ifdef IMPNG_SETJMP_NOT_THREAD_SAFE
  LockSemaphoreInfo(ping_semaphore);
#endif

#ifdef PNG_BENIGN_ERRORS_SUPPORTED
  /* Allow benign errors */
  png_set_benign_errors(ping, 1);
#endif

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
  /* Reject images with too many rows or columns */
  png_set_user_limits(ping,
    (png_uint_32) MagickMin(0x7fffffffL,
        GetMagickResourceLimit(WidthResource)),
    (png_uint_32) MagickMin(0x7fffffffL,
        GetMagickResourceLimit(HeightResource)));
#endif /* PNG_SET_USER_LIMITS_SUPPORTED */

  /*
    Prepare PNG for writing.
  */

  if (mng_info->write_mng != MagickFalse)
  {
     (void) png_permit_mng_features(ping,PNG_ALL_MNG_FEATURES);
# ifdef PNG_WRITE_CHECK_FOR_INVALID_INDEX_SUPPORTED
     /* Disable new libpng-1.5.10 feature when writing a MNG because
      * zero-length PLTE is OK
      */
     png_set_check_for_invalid_index (ping, 0);
# endif
  }

  x=0;

  ping_width=(png_uint_32) image->columns;
  ping_height=(png_uint_32) image->rows;

  if ((mng_info->write_png8 != MagickFalse) ||
      (mng_info->write_png24 != MagickFalse) ||
      (mng_info->write_png32 != MagickFalse))
     image_depth=8;

  if ((mng_info->write_png48 != MagickFalse) ||
      (mng_info->write_png64 != MagickFalse))
     image_depth=16;

  if (mng_info->depth != 0)
     image_depth=mng_info->depth;

  /* Adjust requested depth to next higher valid depth if necessary */
  if (image_depth > 8)
     image_depth=16;

  if ((image_depth > 4) && (image_depth < 8))
     image_depth=8;

  if (image_depth == 3)
     image_depth=4;

  if (logging != MagickFalse)
    {
     (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    width=%.20g",(double) ping_width);
     (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    height=%.20g",(double) ping_height);
     (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    image_matte=%.20g",(double) image->alpha_trait);
     (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    image->depth=%.20g",(double) image->depth);
     (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    Tentative ping_bit_depth=%.20g",(double) image_depth);
    }