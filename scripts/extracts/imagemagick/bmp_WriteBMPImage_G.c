/* ===== EXTRACT: bmp.c ===== */
/* Function: WriteBMPImage (part G) */
/* Lines: 2566–2676 */

          (bmp_info.gamma_scale.z*0x10000));
        if ((image->rendering_intent != UndefinedIntent) ||
            (profile != (StringInfo *) NULL))
          {
            ssize_t
              intent;

            switch ((int) image->rendering_intent)
            {
              case SaturationIntent:
              {
                intent=LCS_GM_BUSINESS;
                break;
              }
              case RelativeIntent:
              {
                intent=LCS_GM_GRAPHICS;
                break;
              }
              case PerceptualIntent:
              {
                intent=LCS_GM_IMAGES;
                break;
              }
              case AbsoluteIntent:
              {
                intent=LCS_GM_ABS_COLORIMETRIC;
                break;
              }
              default:
              {
                intent=0;
                break;
              }
            }
            (void) WriteBlobLSBLong(image,(unsigned int) intent);
            (void) WriteBlobLSBLong(image,(unsigned int) profile_data);
            (void) WriteBlobLSBLong(image,(unsigned int)
              (profile_size+profile_size_pad));
            (void) WriteBlobLSBLong(image,0x00);  /* reserved */
          }
      }
    if (image->storage_class == PseudoClass)
      {
        unsigned char
          *bmp_colormap;

        /*
          Dump colormap to file.
        */
        if (image->debug != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "  Colormap: %.20g entries",(double) image->colors);
        bmp_colormap=(unsigned char *) AcquireQuantumMemory((size_t) 1UL <<
          bmp_info.bits_per_pixel,4*sizeof(*bmp_colormap));
        if (bmp_colormap == (unsigned char *) NULL)
          {
            pixel_info=RelinquishVirtualMemory(pixel_info);
            ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
          }
        q=bmp_colormap;
        for (i=0; i < MagickMin((ssize_t) image->colors,(ssize_t) bmp_info.number_colors); i++)
        {
          *q++=ScaleQuantumToChar(ClampToQuantum(image->colormap[i].blue));
          *q++=ScaleQuantumToChar(ClampToQuantum(image->colormap[i].green));
          *q++=ScaleQuantumToChar(ClampToQuantum(image->colormap[i].red));
          if (type > 2)
            *q++=(unsigned char) 0x0;
        }
        for ( ; i < (ssize_t) 1UL << bmp_info.bits_per_pixel; i++)
        {
          *q++=(unsigned char) 0x00;
          *q++=(unsigned char) 0x00;
          *q++=(unsigned char) 0x00;
          if (type > 2)
            *q++=(unsigned char) 0x00;
        }
        if (type <= 2)
          (void) WriteBlob(image,(size_t) (3*(1L << bmp_info.bits_per_pixel)),
            bmp_colormap);
        else
          (void) WriteBlob(image,(size_t) (4*(1L << bmp_info.bits_per_pixel)),
            bmp_colormap);
        bmp_colormap=(unsigned char *) RelinquishMagickMemory(bmp_colormap);
      }
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  Pixels:  %u bytes",bmp_info.image_size);
    (void) WriteBlob(image,(size_t) bmp_info.image_size,pixels);
    if (profile != (StringInfo *) NULL)
      {
        if (image->debug != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "  Profile:  %g bytes",(double) profile_size+profile_size_pad);
        (void) WriteBlob(image,(size_t) profile_size,
          GetStringInfoDatum(profile));
        if (profile_size_pad > 0)  /* padding for 4 bytes multiple */
          (void) WriteBlob(image,(size_t) profile_size_pad,"\0\0\0");
      }
    pixel_info=RelinquishVirtualMemory(pixel_info);
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    status=SetImageProgress(image,SaveImagesTag,scene++,number_scenes);
    if (status == MagickFalse)
      break;
  } while (image_info->adjoin != MagickFalse);
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  return(status);
}
