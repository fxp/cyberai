// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 75/94



                if (ping_color_type == PNG_COLOR_TYPE_GRAY)
                  {
                    if (image->storage_class == DirectClass)
                      (void) ExportQuantumPixels(image,(CacheView *) NULL,
                        quantum_info,RedQuantum,ping_pixels,exception);

                    else
                      (void) ExportQuantumPixels(image,(CacheView *) NULL,
                        quantum_info,GrayQuantum,ping_pixels,exception);
                  }

                else if (ping_color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
                  {
                    (void) ExportQuantumPixels(image,(CacheView *) NULL,
                      quantum_info,GrayAlphaQuantum,ping_pixels,exception);

                    if (logging != MagickFalse && y == 0)
                      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                           "    Writing GRAY_ALPHA PNG pixels (3)");
                  }

                else if (image_matte != MagickFalse)
                  (void) ExportQuantumPixels(image,(CacheView *) NULL,
                    quantum_info,RGBAQuantum,ping_pixels,exception);

                else
                  (void) ExportQuantumPixels(image,(CacheView *) NULL,
                    quantum_info,RGBQuantum,ping_pixels,exception);

                if (logging != MagickFalse && y == 0)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                      "    Writing row of pixels (3)");

                png_write_row(ping,ping_pixels);

                status=SetImageProgress(image,SaveImageTag,
                  (MagickOffsetType) (pass * (ssize_t) image->rows + y),
                  (MagickSizeType) num_passes * image->rows);

                if (status == MagickFalse)
                  break;
              }
            }

          else
            /* not ((image_depth > 8) ||
                mng_info->write_png24 || mng_info->write_png32 ||
                mng_info->write_png48 || mng_info->write_png64 ||
                (!mng_info->write_png8 && !mng_info->IsPalette))
             */
            {
              if ((ping_color_type != PNG_COLOR_TYPE_GRAY) &&
                  (ping_color_type != PNG_COLOR_TYPE_GRAY_ALPHA))
                {
                  if (logging != MagickFalse)
                    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                      "  pass %d, Image Is not GRAY or GRAY_ALPHA",pass);

                  (void) SetQuantumDepth(image,quantum_info,8);
                  image_depth=8;
                }

              for (y=0; y < (ssize_t) image->rows; y++)
              {
                if (logging != MagickFalse && y == 0)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "  pass %d, Image Is RGB, 16-bit GRAY, or GRAY_ALPHA",
                    pass);

                p=GetVirtualPixels(image,0,y,image->columns,1, exception);

                if (p == (const Quantum *) NULL)
                  break;

                if (ping_color_type == PNG_COLOR_TYPE_GRAY)
                  {
                    (void) SetQuantumDepth(image,quantum_info,image->depth);

                    (void) ExportQuantumPixels(image,(CacheView *) NULL,
                       quantum_info,GrayQuantum,ping_pixels,exception);
                  }

                else if (ping_color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
                  {
                    if (logging != MagickFalse && y == 0)
                      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                           "  Writing GRAY_ALPHA PNG pixels (4)");

                    (void) ExportQuantumPixels(image,(CacheView *) NULL,
                         quantum_info,GrayAlphaQuantum,ping_pixels,
                         exception);
                  }

                else
                  {
                    (void) ExportQuantumPixels(image,(CacheView *) NULL,
                      quantum_info,IndexQuantum,ping_pixels,exception);

                    if (logging != MagickFalse && y <= 2)
                    {
                      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                          "  Writing row of non-gray pixels (4)");

                      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                          "  ping_pixels[0]=%d,ping_pixels[1]=%d",
                          (int)ping_pixels[0],(int)ping_pixels[1]);
                    }
                  }
                png_write_row(ping,ping_pixels);

                status=SetImageProgress(image,SaveImageTag,
                  (MagickOffsetType) pass * (ssize_t) image->rows + y,
                  (MagickSizeType) num_passes * image->rows);

                if (status == MagickFalse)
                  break;
              }
            }
          }
        }
    }

  if (quantum_info != (QuantumInfo *) NULL)
    quantum_info=DestroyQuantumInfo(quantum_info);