/* ===== EXTRACT: webp.c ===== */
/* Function: WriteWEBPImage (part B) */
/* Lines: 1237–1283 */

          if ((next->rows != image->rows) || (next->columns != image->columns))
            {
              coalesce_image=CoalesceImages(image,exception);
              break;
            }
          next=GetNextImageInList(next);
        }
        if (coalesce_image != (Image *) NULL)
          {
            status=WriteAnimatedWEBPImage(image_info,coalesce_image,&configure,
              &webp_data,exception);
            (void) DestroyImageList(coalesce_image);
          }
        else
          status=WriteAnimatedWEBPImage(image_info,image,&configure,&webp_data,
            exception);
      }
    else
      {
        WebPMemoryWriterInit(&writer);
        status=WriteSingleWEBPImage(image_info,image,&configure,&writer,
          exception);
        if (status == MagickFalse)
          WebPMemoryWriterClear(&writer);
        else
          {
            webp_data.bytes=writer.mem;
            webp_data.size=writer.size;
          }
      }
    if (status != MagickFalse)
      status=WriteWEBPImageProfile(image,&webp_data,exception);
    if (status != MagickFalse)
      (void) WriteBlob(image,webp_data.size,webp_data.bytes);
    WebPDataClear(&webp_data);
  }
#else
  WebPMemoryWriterInit(&writer);
  status=WriteSingleWEBPImage(image_info,image,&configure,&writer,exception);
  if (status != MagickFalse)
    (void) WriteBlob(image,writer.size,writer.mem);
  WebPMemoryWriterClear(&writer);
#endif
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  return(status);
}
