// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jxl.c
// Segment 6/11



        if (image_count++ != 0)
          {
            JXLAddProfilesToImage(image,&exif_profile,&xmp_profile,exception);
            /*
              Allocate next image structure.
            */
            AcquireNextImage(image_info,image,exception);
            if (GetNextImageInList(image) == (Image *) NULL)
              break;
            image=SyncNextImageInList(image);
            JXLInitImage(image,&basic_info);
          }
        (void) memset(&frame_header,0,sizeof(frame_header));
        if (JxlDecoderGetFrameHeader(jxl_info,&frame_header) == JXL_DEC_SUCCESS)
          image->delay=(size_t) frame_header.duration;
        if ((basic_info.have_animation == JXL_TRUE) &&
            (basic_info.alpha_bits != 0))
          image->dispose=BackgroundDispose;
        break;
      }
      case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
      {
        status=SetImageExtent(image,image->columns,image->rows,exception);
        if (status == MagickFalse)
          {
            jxl_status=JXL_DEC_ERROR;
            break;
          }
        (void) ResetImagePixels(image,exception);
        JXLSetFormat(image,&pixel_format,exception);
        if (extent == 0)
          {
            jxl_status=JxlDecoderImageOutBufferSize(jxl_info,&pixel_format,
              &extent);
            if (jxl_status != JXL_DEC_SUCCESS)
              break;
            output_buffer=(unsigned char *) AcquireQuantumMemory(extent,
              sizeof(*output_buffer));
            if (output_buffer == (unsigned char *) NULL)
              {
                (void) ThrowMagickException(exception,GetMagickModule(),
                  CoderError,"MemoryAllocationFailed","`%s'",image->filename);
                break;
              }
          }
        jxl_status=JxlDecoderSetImageOutBuffer(jxl_info,&pixel_format,
          output_buffer,extent);
        if (jxl_status == JXL_DEC_SUCCESS)
          jxl_status=JXL_DEC_NEED_IMAGE_OUT_BUFFER;
        break;
      }
      case JXL_DEC_FULL_IMAGE:
      {
        const char
          *map = "RGB";

        StorageType
          type;

        if (output_buffer == (unsigned char *) NULL)
          {
            (void) ThrowMagickException(exception,GetMagickModule(),
              CorruptImageError,"UnableToReadImageData","`%s'",image->filename);
            break;
          }
        type=JXLDataTypeToStorageType(image,pixel_format.data_type,exception);
        if (type == UndefinedPixel)
          {
            (void) ThrowMagickException(exception,GetMagickModule(),
              CorruptImageError,"Unsupported data type","`%s'",image->filename);
            break;
          }
        if ((image->alpha_trait & BlendPixelTrait) != 0)
          map="RGBA";
        if (IsGrayColorspace(image->colorspace) != MagickFalse)
          {
            map="I";
            if ((image->alpha_trait & BlendPixelTrait) != 0)
              map="IA";
          }
        status=ImportImagePixels(image,0,0,image->columns,image->rows,map,
          type,output_buffer,exception);
        if (status == MagickFalse)
          jxl_status=JXL_DEC_ERROR;
        break;
      }
      case JXL_DEC_BOX:
      {
        JxlBoxType
          type;

        uint64_t
          size;