/* ===== pngrtran.c [png_do_compose part1] ===== */
/* Lines: 3195–3600 */
/* File: pngrtran.c — libpng 1.6.45 */

png_do_compose(png_row_infop row_info, png_bytep row, png_structrp png_ptr)
{
#ifdef PNG_READ_GAMMA_SUPPORTED
   png_const_bytep gamma_table = png_ptr->gamma_table;
   png_const_bytep gamma_from_1 = png_ptr->gamma_from_1;
   png_const_bytep gamma_to_1 = png_ptr->gamma_to_1;
   png_const_uint_16pp gamma_16 = png_ptr->gamma_16_table;
   png_const_uint_16pp gamma_16_from_1 = png_ptr->gamma_16_from_1;
   png_const_uint_16pp gamma_16_to_1 = png_ptr->gamma_16_to_1;
   int gamma_shift = png_ptr->gamma_shift;
   int optimize = (png_ptr->flags & PNG_FLAG_OPTIMIZE_ALPHA) != 0;
#endif

   png_bytep sp;
   png_uint_32 i;
   png_uint_32 row_width = row_info->width;
   int shift;

   png_debug(1, "in png_do_compose");

   switch (row_info->color_type)
   {
      case PNG_COLOR_TYPE_GRAY:
      {
         switch (row_info->bit_depth)
         {
            case 1:
            {
               sp = row;
               shift = 7;
               for (i = 0; i < row_width; i++)
               {
                  if ((png_uint_16)((*sp >> shift) & 0x01)
                     == png_ptr->trans_color.gray)
                  {
                     unsigned int tmp = *sp & (0x7f7f >> (7 - shift));
                     tmp |=
                         (unsigned int)(png_ptr->background.gray << shift);
                     *sp = (png_byte)(tmp & 0xff);
                  }

                  if (shift == 0)
                  {
                     shift = 7;
                     sp++;
                  }

                  else
                     shift--;
               }
               break;
            }

            case 2:
            {
#ifdef PNG_READ_GAMMA_SUPPORTED
               if (gamma_table != NULL)
               {
                  sp = row;
                  shift = 6;
                  for (i = 0; i < row_width; i++)
                  {
                     if ((png_uint_16)((*sp >> shift) & 0x03)
                         == png_ptr->trans_color.gray)
                     {
                        unsigned int tmp = *sp & (0x3f3f >> (6 - shift));
                        tmp |=
                           (unsigned int)png_ptr->background.gray << shift;
                        *sp = (png_byte)(tmp & 0xff);
                     }

                     else
                     {
                        unsigned int p = (*sp >> shift) & 0x03;
                        unsigned int g = (gamma_table [p | (p << 2) |
                            (p << 4) | (p << 6)] >> 6) & 0x03;
                        unsigned int tmp = *sp & (0x3f3f >> (6 - shift));
                        tmp |= (unsigned int)(g << shift);
                        *sp = (png_byte)(tmp & 0xff);
                     }

                     if (shift == 0)
                     {
                        shift = 6;
                        sp++;
                     }

                     else
                        shift -= 2;
                  }
               }

               else
#endif
               {
                  sp = row;
                  shift = 6;
                  for (i = 0; i < row_width; i++)
                  {
                     if ((png_uint_16)((*sp >> shift) & 0x03)
                         == png_ptr->trans_color.gray)
                     {
                        unsigned int tmp = *sp & (0x3f3f >> (6 - shift));
                        tmp |=
                            (unsigned int)png_ptr->background.gray << shift;
                        *sp = (png_byte)(tmp & 0xff);
                     }

                     if (shift == 0)
                     {
                        shift = 6;
                        sp++;
                     }

                     else
                        shift -= 2;
                  }
               }
               break;
            }

            case 4:
            {
#ifdef PNG_READ_GAMMA_SUPPORTED
               if (gamma_table != NULL)
               {
                  sp = row;
                  shift = 4;
                  for (i = 0; i < row_width; i++)
                  {
                     if ((png_uint_16)((*sp >> shift) & 0x0f)
                         == png_ptr->trans_color.gray)
             
/* ... truncated ... */