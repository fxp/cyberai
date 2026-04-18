// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 18/18



                 if (((ssize_t) length-offset) < 255)
                   block_length=(size_t) ((ssize_t) length-offset);
                 else
                   block_length=255;
                 (void) WriteBlobByte(image,(unsigned char) block_length);
                 (void) WriteBlob(image,(size_t) block_length,datum+offset);
                 offset+=(ssize_t) block_length;
               }
               (void) WriteBlobByte(image,(unsigned char) 0x00);
            }
          }
        }
      }
    (void) WriteBlobByte(image,',');  /* image separator */
    /*
      Write the image header.
    */
    page.x=image->page.x;
    page.y=image->page.y;
    if ((image->page.width != 0) && (image->page.height != 0))
      page=image->page;
    (void) WriteBlobLSBShort(image,(unsigned short) (page.x < 0 ? 0 : page.x));
    (void) WriteBlobLSBShort(image,(unsigned short) (page.y < 0 ? 0 : page.y));
    (void) WriteBlobLSBShort(image,(unsigned short) image->columns);
    (void) WriteBlobLSBShort(image,(unsigned short) image->rows);
    c=0x00;
    if (write_info->interlace != NoInterlace)
      c|=0x40;  /* pixel data is interlaced */
    for (j=0; j < (ssize_t) (3*image->colors); j++)
      if (colormap[j] != global_colormap[j])
        break;
    if (j == (ssize_t) (3*image->colors))
      (void) WriteBlobByte(image,(unsigned char) c);
    else
      {
        c|=0x80;
        c|=(int) (bits_per_pixel-1);   /* size of local colormap */
        (void) WriteBlobByte(image,(unsigned char) c);
        length=(size_t) (3*(one << bits_per_pixel));
        (void) WriteBlob(image,length,colormap);
      }
    /*
      Write the image data.
    */
    c=(int) MagickMax(bits_per_pixel,2);
    (void) WriteBlobByte(image,(unsigned char) c);
    status=EncodeImage(write_info,image,(size_t) MagickMax(bits_per_pixel,2)+1,
      exception);
    if (status == MagickFalse)
      {
        global_colormap=(unsigned char *) RelinquishMagickMemory(
          global_colormap);
        colormap=(unsigned char *) RelinquishMagickMemory(colormap);
        write_info=DestroyImageInfo(write_info);
        ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
      }
    (void) WriteBlobByte(image,(unsigned char) 0x00);
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    scene++;
    status=SetImageProgress(image,SaveImagesTag,scene,number_scenes);
    if (status == MagickFalse)
      break;
  } while (write_info->adjoin != MagickFalse);
  (void) WriteBlobByte(image,';'); /* terminator */
  global_colormap=(unsigned char *) RelinquishMagickMemory(global_colormap);
  colormap=(unsigned char *) RelinquishMagickMemory(colormap);
  write_info=DestroyImageInfo(write_info);
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  return(status);
}
