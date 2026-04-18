// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 73/94



                LogPNGChunk(logging,chunk,length);
                (void) WriteBlobMSBULong(image,length);
                (void) WriteBlob(image,4,chunk);
                (void) WriteBlob(image,length,data);
                (void) WriteBlobMSBULong(image,crc32(crc32(0,chunk,4), data,
                  (uInt) length));
                ping_profile=DestroyStringInfo(ping_profile);
                break;
             }
         }
       name=GetNextImageProfile(image);
     }
  }

  png_write_info(ping,ping_info);

  /* write orNT if image->orientation is defined */
  if (image->orientation != UndefinedOrientation)
    {
      unsigned char
        chunk[6];
      (void) WriteBlobMSBULong(image,1L);  /* data length=1 */
      PNGType(chunk,mng_orNT);
      LogPNGChunk(logging,mng_orNT,1L);
      /* PNG uses Exif orientation values */
      chunk[4]=(unsigned char) Magick_Orientation_to_Exif_Orientation(image->orientation);
      (void) WriteBlob(image,5,chunk);
      (void) WriteBlobMSBULong(image,crc32(0,chunk,5));
    }

  ping_wrote_caNv = MagickFalse;

  /* write caNv chunk */
  if (mng_info->exclude_caNv == MagickFalse)
    {
      if ((image->page.width != 0 && image->page.width != image->columns) ||
          (image->page.height != 0 && image->page.height != image->rows) ||
          image->page.x != 0 || image->page.y != 0)
        {
          unsigned char
            chunk[20];

          (void) WriteBlobMSBULong(image,16L);  /* data length=8 */
          PNGType(chunk,mng_caNv);
          LogPNGChunk(logging,mng_caNv,16L);
          PNGLong(chunk+4,(png_uint_32) image->page.width);
          PNGLong(chunk+8,(png_uint_32) image->page.height);
          PNGsLong(chunk+12,(png_int_32) image->page.x);
          PNGsLong(chunk+16,(png_int_32) image->page.y);
          (void) WriteBlob(image,20,chunk);
          (void) WriteBlobMSBULong(image,crc32(0,chunk,20));
          ping_wrote_caNv = MagickTrue;
        }
    }

#if defined(PNG_oFFs_SUPPORTED)
  if (mng_info->exclude_oFFs == MagickFalse && ping_wrote_caNv == MagickFalse)
    {
      if (image->page.x || image->page.y)
        {
           png_set_oFFs(ping,ping_info,(png_int_32) image->page.x,
              (png_int_32) image->page.y, 0);

           if (logging != MagickFalse)
             (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "    Setting up oFFs chunk with x=%d, y=%d, units=0",
                 (int) image->page.x, (int) image->page.y);
        }
    }
#endif

#if (PNG_LIBPNG_VER == 10206)
    /* avoid libpng-1.2.6 bug by setting PNG_HAVE_IDAT flag */
#define PNG_HAVE_IDAT               0x04
    ping->mode |= PNG_HAVE_IDAT;
#undef PNG_HAVE_IDAT
#endif

  png_set_packing(ping);
  /*
    Allocate memory.
  */
  rowbytes=image->columns;
  if (image_depth > 8)
    rowbytes*=2;
  switch (ping_color_type)
    {
      case PNG_COLOR_TYPE_RGB:
        rowbytes*=3;
        break;

      case PNG_COLOR_TYPE_GRAY_ALPHA:
        rowbytes*=2;
        break;

      case PNG_COLOR_TYPE_RGBA:
        rowbytes*=4;
        break;

      default:
        break;
    }

  if (logging != MagickFalse)
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "  Writing PNG image data");

      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "    Allocating %.20g bytes of memory for pixels",(double) rowbytes);
    }
  pixel_info=AcquireVirtualMemory(rowbytes,GetPixelChannels(image)*
    sizeof(*ping_pixels));
  if (pixel_info == (MemoryInfo *) NULL)
    png_error(ping,"Allocation of memory for pixels failed");
  ping_pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
  (void) memset(ping_pixels,0,rowbytes*GetPixelChannels(image)*
    sizeof(*ping_pixels));
  /*
    Initialize image scanlines.
  */
  quantum_info=AcquireQuantumInfo(image_info,image);
  if (quantum_info == (QuantumInfo *) NULL)
    png_error(ping,"Memory allocation for quantum_info failed");
  (void) SetQuantumFormat(image,quantum_info,UndefinedQuantumFormat);
  (void) SetQuantumDepth(image,quantum_info,image_depth);
  (void) SetQuantumEndian(image,quantum_info,MSBEndian);
  num_passes=png_set_interlace_handling(ping);

  if ((mng_info->colortype-1 == PNG_COLOR_TYPE_PALETTE) ||
      ((mng_info->write_png8 == MagickFalse) &&
       (mng_info->write_png24 == MagickFalse) &&
       (mng_info->write_png48 == MagickFalse) &&
       (mng_info->write_png64 == MagickFalse) &&
       (mng_info->write_png32 == MagickFalse) &&
       (image_matte == MagickFalse) &&
       (ping_have_non_bw == MagickFalse) &&
       ((mng_info->is_palette != MagickFalse) ||
        (image_info->type == BilevelType))))
     {
      /* Palette, Bilevel, or Opaque Monochrome */
      QuantumType
        quantum_type;

      const Quantum
        *p;