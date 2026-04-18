// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 11/24

           bytes_per_line=4*(image->columns);
            for (y=(ssize_t) image->rows-1; y >= 0; y--)
            {
              p=pixels+((ssize_t) image->rows-y-1)*(ssize_t) bytes_per_line;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                if (*(p+3) != 0)
                  {
                    image->alpha_trait=BlendPixelTrait;
                    y=-1;
                    break;
                  }
                p+=(ptrdiff_t) 4;
              }
            }
          }
        bmp_info.alpha_mask=image->alpha_trait != UndefinedPixelTrait ?
          0xff000000U : 0U;
        bmp_info.red_mask=0x00ff0000U;
        bmp_info.green_mask=0x0000ff00U;
        bmp_info.blue_mask=0x000000ffU;
        if (bmp_info.bits_per_pixel == 16)
          {
            /*
              RGB555.
            */
            bmp_info.red_mask=0x00007c00U;
            bmp_info.green_mask=0x000003e0U;
            bmp_info.blue_mask=0x0000001fU;
          }
      }
    (void) memset(&shift,0,sizeof(shift));
    (void) memset(&quantum_bits,0,sizeof(quantum_bits));
    if ((bmp_info.bits_per_pixel == 16) || (bmp_info.bits_per_pixel == 32))
      {
        unsigned int
          sample;