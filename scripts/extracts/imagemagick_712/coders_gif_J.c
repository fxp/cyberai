// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 10/18



                    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                      "      profile name=%s",name);
                    profile=BlobToProfileStringInfo(name,info,(size_t) info_length,
                      exception);
                    if (profile != (StringInfo *) NULL)
                      {
                        if (profiles == (LinkedListInfo *) NULL)
                          profiles=NewLinkedList(0);
                        (void) AppendValueToLinkedList(profiles,profile);
                      }
                  }
                info=(unsigned char *) RelinquishMagickMemory(info);
              }
            break;
          }
          default:
          {
            while (ReadBlobBlock(image,buffer) != 0) ;
            break;
          }
        }
      }
    if (c != (unsigned char) ',')
      continue;
    image_count++;
    if (image_count != 1)
      {
        /*
          Allocate next image structure.
        */
        AcquireNextImage(image_info,image,exception);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            status=MagickFalse;
            break;
          }
        image=SyncNextImageInList(image);
      }
    /*
      Read image attributes.
    */
    meta_image->page.x=(ssize_t) ReadBlobLSBShort(image);
    meta_image->page.y=(ssize_t) ReadBlobLSBShort(image);
    meta_image->scene=image->scene;
    (void) CloneImageProperties(image,meta_image);
    DestroyImageProperties(meta_image);
    image->storage_class=PseudoClass;
    image->compression=LZWCompression;
    image->columns=ReadBlobLSBShort(image);
    image->rows=ReadBlobLSBShort(image);
    image->depth=8;
    flag=(unsigned char) ReadBlobByte(image);
    image->interlace=BitSet((int) flag,0x40) != 0 ? GIFInterlace : NoInterlace;
    local_colors=BitSet((int) flag,0x80) == 0 ? global_colors : one <<
      ((size_t) (flag & 0x07)+1);
    image->colors=local_colors;
    if (opacity == (ssize_t) image->colors)
      image->colors++;
    else if (opacity > (ssize_t) image->colors)
      opacity=(-1);
    image->ticks_per_second=100;
    image->alpha_trait=opacity >= 0 ? BlendPixelTrait : UndefinedPixelTrait;
    if ((image->columns == 0) || (image->rows == 0))
      ThrowGIFException(CorruptImageError,"NegativeOrZeroImageSize");
    /*
      Initialize colormap.
    */
    if (AcquireImageColormap(image,image->colors,exception) == MagickFalse)
      ThrowGIFException(ResourceLimitError,"MemoryAllocationFailed");
    if (BitSet((int) flag,0x80) == 0)
      {
        /*
          Use global colormap.
        */
        p=global_colormap;
        for (i=0; i < (ssize_t) image->colors; i++)
        {
          image->colormap[i].red=(double) ScaleCharToQuantum(*p++);
          image->colormap[i].green=(double) ScaleCharToQuantum(*p++);
          image->colormap[i].blue=(double) ScaleCharToQuantum(*p++);
          if (i == opacity)
            {
              image->colormap[i].alpha=(double) TransparentAlpha;
              image->transparent_color=image->colormap[opacity];
            }
        }
        if (image->colors > 0)
          image->background_color=image->colormap[MagickMin((ssize_t)
            background,(ssize_t) image->colors-1)];
      }
    else
      {
        unsigned char
          *colormap;