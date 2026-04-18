// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jxl.c
// Segment 11/11



        frame_header.duration=(uint32_t) image->delay;
        if ((image->previous == (Image *) NULL) ||
            (image->previous->dispose == BackgroundDispose) ||
            (image->previous->dispose == PreviousDispose))
          {
            frame_header.layer_info.blend_info.blendmode=JXL_BLEND_REPLACE;
            frame_header.layer_info.blend_info.source=0;
          }
        else
          {
            frame_header.layer_info.blend_info.blendmode=JXL_BLEND_BLEND;
            frame_header.layer_info.blend_info.source=1;
          }
        frame_header.layer_info.save_as_reference=1;
        if ((image->page.width != 0) && (image->page.height != 0))
          {
            frame_header.layer_info.have_crop=JXL_TRUE;
            frame_header.layer_info.crop_x0=(int32_t) image->page.x;
            frame_header.layer_info.crop_y0=(int32_t) image->page.y;
            frame_header.layer_info.xsize=(uint32_t) image->columns;
            frame_header.layer_info.ysize=(uint32_t) image->rows;
          }
        jxl_status=JxlEncoderSetFrameHeader(frame_settings,&frame_header);
        if (jxl_status != JXL_ENC_SUCCESS)
          break;
        if (basic_info.num_extra_channels > 0)
          {
            JxlEncoderInitBlendInfo(&alpha_blend_info);
            alpha_blend_info.blendmode=frame_header.layer_info.blend_info.blendmode;
            alpha_blend_info.source=frame_header.layer_info.blend_info.source;
            (void) JxlEncoderSetExtraChannelBlendInfo(frame_settings,0,&alpha_blend_info);
          }
      }
    pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
    if (IsGrayColorspace(image->colorspace) != MagickFalse)
      status=ExportImagePixels(image,0,0,image->columns,image->rows,
        ((image->alpha_trait & BlendPixelTrait) != 0) ? "IA" : "I",
        JXLDataTypeToStorageType(image,pixel_format.data_type,exception),
        pixels,exception);
    else
      status=ExportImagePixels(image,0,0,image->columns,image->rows,
        ((image->alpha_trait & BlendPixelTrait) != 0) ? "RGBA" : "RGB",
        JXLDataTypeToStorageType(image,pixel_format.data_type,exception),
        pixels,exception);
    if (status == MagickFalse)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),CoderError,
          "MemoryAllocationFailed","`%s'",image->filename);
        status=MagickFalse;
        break;
      }
    if (jxl_status != JXL_ENC_SUCCESS)
      break;
    jxl_status=JxlEncoderAddImageFrame(frame_settings,&pixel_format,pixels,
      bytes_per_row*image->rows);
    if (jxl_status != JXL_ENC_SUCCESS)
      break;
    next=GetNextImageInList(image);
    if (next == (Image*) NULL)
      break;
    if (JXLSameFrameType(image,next) == MagickFalse)
      {
       (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
         "FramesNotSameDimensions","`%s'",image->filename);
       status=MagickFalse;
       break;
      }
    pixel_info=RelinquishVirtualMemory(pixel_info);
    image=SyncNextImageInList(image);
  } while (image_info->adjoin != MagickFalse);
  if (pixel_info != (MemoryInfo *) NULL)
    pixel_info=RelinquishVirtualMemory(pixel_info);
  if (jxl_status == JXL_ENC_SUCCESS)
    {
      unsigned char
        *output_buffer;

      JxlEncoderCloseInput(jxl_info);
      output_buffer=(unsigned char *) AcquireQuantumMemory(
        MagickMaxBufferExtent,sizeof(*output_buffer));
      if (output_buffer == (unsigned char *) NULL)
        {
          JxlThreadParallelRunnerDestroy(runner);
          JxlEncoderDestroy(jxl_info);
          ThrowWriterException(CoderError,"MemoryAllocationFailed");
        }
      jxl_status=JXL_ENC_NEED_MORE_OUTPUT;
      while (jxl_status == JXL_ENC_NEED_MORE_OUTPUT)
      {
        size_t
          extent;

        ssize_t
          count;

        unsigned char
          *p;

        /*
          Encode the pixel stream.
        */
        extent=MagickMaxBufferExtent;
        p=output_buffer;
        jxl_status=JxlEncoderProcessOutput(jxl_info,&p,&extent);
        count=WriteBlob(image,MagickMaxBufferExtent-extent,output_buffer);
        if (count != (ssize_t) (MagickMaxBufferExtent-extent))
          {
            jxl_status=JXL_ENC_ERROR;
            break;
          }
      }
      output_buffer=(unsigned char *) RelinquishMagickMemory(output_buffer);
    }
  JxlThreadParallelRunnerDestroy(runner);
  JxlEncoderDestroy(jxl_info);
  if (jxl_status != JXL_ENC_SUCCESS)
    ThrowWriterException(CoderError,"UnableToWriteImageData");
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  return(status);
}
#endif
