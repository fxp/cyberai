// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 11/18



        /*
          Read local colormap.
        */
        colormap=(unsigned char *) AcquireQuantumMemory((size_t)
          MagickMax(local_colors,256),3UL*sizeof(*colormap));
        if (colormap == (unsigned char *) NULL)
          ThrowGIFException(ResourceLimitError,"MemoryAllocationFailed");
        (void) memset(colormap,0,3*MagickMax(local_colors,256)*
          sizeof(*colormap));
        count=ReadBlob(image,(3*local_colors)*sizeof(*colormap),colormap);
        if (count != (ssize_t) (3*local_colors))
          {
            colormap=(unsigned char *) RelinquishMagickMemory(colormap);
            ThrowGIFException(CorruptImageError,"InsufficientImageDataInFile");
          }
        p=colormap;
        for (i=0; i < (ssize_t) image->colors; i++)
        {
          image->colormap[i].red=(double) ScaleCharToQuantum(*p++);
          image->colormap[i].green=(double) ScaleCharToQuantum(*p++);
          image->colormap[i].blue=(double) ScaleCharToQuantum(*p++);
          if (i == opacity)
            image->colormap[i].alpha=(double) TransparentAlpha;
        }
        colormap=(unsigned char *) RelinquishMagickMemory(colormap);
      }
    if (image->gamma == 1.0)
      {
        for (i=0; i < (ssize_t) image->colors; i++)
          if (IsPixelInfoGray(image->colormap+i) == MagickFalse)
            break;
        (void) SetImageColorspace(image,i == (ssize_t) image->colors ?
          GRAYColorspace : RGBColorspace,exception);
      }
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    status=SetImageExtent(image,image->columns,image->rows,exception);
    if (status == MagickFalse)
      {
        if (profiles != (LinkedListInfo *) NULL)
          profiles=DestroyLinkedList(profiles,DestroyGIFProfile);
        global_colormap=(unsigned char *) RelinquishMagickMemory(
          global_colormap);
        meta_image=DestroyImage(meta_image);
        return(DestroyImageList(image));
      }
    /*
      Decode image.
    */
    if (image_info->ping != MagickFalse)
      status=PingGIFImage(image,exception);
    else
      status=DecodeImage(image,opacity,exception);
    if ((image_info->ping == MagickFalse) && (status == MagickFalse))
      ThrowGIFException(CorruptImageError,"CorruptImage");
    if (profiles != (LinkedListInfo *) NULL)
      {
        StringInfo
          *profile;