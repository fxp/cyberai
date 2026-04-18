// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 86/94



          /*
             Write JNG cHRM chunk
          */
          (void) WriteBlobMSBULong(image,32L);
          PNGType(chunk,mng_cHRM);
          LogPNGChunk(logging,mng_cHRM,32L);
          primary=image->chromaticity.white_point;
          PNGLong(chunk+4,(png_uint_32) (100000*primary.x+0.5));
          PNGLong(chunk+8,(png_uint_32) (100000*primary.y+0.5));
          primary=image->chromaticity.red_primary;
          PNGLong(chunk+12,(png_uint_32) (100000*primary.x+0.5));
          PNGLong(chunk+16,(png_uint_32) (100000*primary.y+0.5));
          primary=image->chromaticity.green_primary;
          PNGLong(chunk+20,(png_uint_32) (100000*primary.x+0.5));
          PNGLong(chunk+24,(png_uint_32) (100000*primary.y+0.5));
          primary=image->chromaticity.blue_primary;
          PNGLong(chunk+28,(png_uint_32) (100000*primary.x+0.5));
          PNGLong(chunk+32,(png_uint_32) (100000*primary.y+0.5));
          (void) WriteBlob(image,36,chunk);
          (void) WriteBlobMSBULong(image,crc32(0,chunk,36));
        }
    }

  if (image->resolution.x && image->resolution.y &&
      mng_info->equal_physs == MagickFalse)
    {
      /*
         Write JNG pHYs chunk
      */
      (void) WriteBlobMSBULong(image,9L);
      PNGType(chunk,mng_pHYs);
      LogPNGChunk(logging,mng_pHYs,9L);
      if (image->units == PixelsPerInchResolution)
        {
          PNGLong(chunk+4,(png_uint_32)
            (image->resolution.x*100.0/2.54+0.5));

          PNGLong(chunk+8,(png_uint_32)
            (image->resolution.y*100.0/2.54+0.5));

          chunk[12]=1;
        }

      else
        {
          if (image->units == PixelsPerCentimeterResolution)
            {
              PNGLong(chunk+4,(png_uint_32)
                (image->resolution.x*100.0+0.5));

              PNGLong(chunk+8,(png_uint_32)
                (image->resolution.y*100.0+0.5));

              chunk[12]=1;
            }

          else
            {
              PNGLong(chunk+4,(png_uint_32) (image->resolution.x+0.5));
              PNGLong(chunk+8,(png_uint_32) (image->resolution.y+0.5));
              chunk[12]=0;
            }
        }
      (void) WriteBlob(image,13,chunk);
      (void) WriteBlobMSBULong(image,crc32(0,chunk,13));
    }

  if ((mng_info->write_mng == MagickFalse) && (image->page.x || image->page.y))
    {
      /*
         Write JNG oFFs chunk
      */
      (void) WriteBlobMSBULong(image,9L);
      PNGType(chunk,mng_oFFs);
      LogPNGChunk(logging,mng_oFFs,9L);
      PNGsLong(chunk+4,(png_int_32) (image->page.x));
      PNGsLong(chunk+8,(png_int_32) (image->page.y));
      chunk[12]=0;
      (void) WriteBlob(image,13,chunk);
      (void) WriteBlobMSBULong(image,crc32(0,chunk,13));
    }

  if (transparent != 0)
    {
      if (jng_alpha_compression_method==0)
        {
          ssize_t
            i;

          size_t
            len;

          /* Write IDAT chunk header */
          if (logging != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Write IDAT chunks from blob, length=%.20g.",(double)
              length);

          /* Copy IDAT chunks */
          len=0;
          p=blob+8;
          for (i=8; i<(ssize_t) length; i+=(ssize_t) len+12)
          {
            len=(((unsigned int) *(p    ) & 0xff) << 24) +
                (((unsigned int) *(p + 1) & 0xff) << 16) +
                (((unsigned int) *(p + 2) & 0xff) <<  8) +
                (((unsigned int) *(p + 3) & 0xff)      ) ;
            p+=(ptrdiff_t) 4;

            if (*(p)==73 && *(p+1)==68 && *(p+2)==65 && *(p+3)==84) /* IDAT */
              {
                /* Found an IDAT chunk. */
                (void) WriteBlobMSBULong(image,len);
                LogPNGChunk(logging,mng_IDAT,len);
                (void) WriteBlob(image,len+4,p);
                (void) WriteBlobMSBULong(image, crc32(0,p,(uInt) len+4));
              }

            else
              {
                if (logging != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "    Skipping %c%c%c%c chunk, length=%.20g.",
                    *(p),*(p+1),*(p+2),*(p+3),(double) len);
              }
            p+=(ptrdiff_t) (8+len);
          }
        }
      else if (length != 0)
        {
          /* Write JDAA chunk header */
          if (logging != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Write JDAA chunk, length=%.20g.",(double) length);
          (void) WriteBlobMSBULong(image,(size_t) length);
          PNGType(chunk,mng_JDAA);
          LogPNGChunk(logging,mng_JDAA,length);
          /* Write JDAT chunk(s) data */
          (void) WriteBlob(image,4,chunk);
          (void) WriteBlob(image,length,blob);
          (void) WriteBlobMSBULong(image,crc32(crc32(0,chunk,4),blob,
             (uInt) length));
        }
      blob=(unsigned char *) RelinquishMagickMemory(blob);
    }