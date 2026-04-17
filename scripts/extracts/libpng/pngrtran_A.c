/* ===== EXTRACT: pngrtran.c [set_quantize+gamma] ===== */
/* File: libpng 1.6.45 */

/* --- png_set_quantize + gamma functions (L426-870) --- */
png_set_quantize(png_structrp png_ptr, png_colorp palette,
    int num_palette, int maximum_colors, png_const_uint_16p histogram,
    int full_quantize)
{
   png_debug(1, "in png_set_quantize");

   if (png_rtran_ok(png_ptr, 0) == 0)
      return;

   png_ptr->transformations |= PNG_QUANTIZE;

   if (full_quantize == 0)
   {
      int i;

      png_ptr->quantize_index = (png_bytep)png_malloc(png_ptr,
          (png_alloc_size_t)num_palette);
      for (i = 0; i < num_palette; i++)
         png_ptr->quantize_index[i] = (png_byte)i;
   }

   if (num_palette > maximum_colors)
   {
      if (histogram != NULL)
      {
         /* This is easy enough, just throw out the least used colors.
          * Perhaps not the best solution, but good enough.
          */

         int i;

         /* Initialize an array to sort colors */
         png_ptr->quantize_sort = (png_bytep)png_malloc(png_ptr,
             (png_alloc_size_t)num_palette);

         /* Initialize the quantize_sort array */
         for (i = 0; i < num_palette; i++)
            png_ptr->quantize_sort[i] = (png_byte)i;

         /* Find the least used palette entries by starting a
          * bubble sort, and running it until we have sorted
          * out enough colors.  Note that we don't care about
          * sorting all the colors, just finding which are
          * least used.
          */

         for (i = num_palette - 1; i >= maximum_colors; i--)
         {
            int done; /* To stop early if the list is pre-sorted */
            int j;

            done = 1;
            for (j = 0; j < i; j++)
            {
               if (histogram[png_ptr->quantize_sort[j]]
                   < histogram[png_ptr->quantize_sort[j + 1]])
               {
                  png_byte t;

                  t = png_ptr->quantize_sort[j];
                  png_ptr->quantize_sort[j] = png_ptr->quantize_sort[j + 1];
                  png_ptr->quantize_sort[j + 1] = t;
                  done = 0;
               }
            }

            if (done != 0)
               break;
         }

         /* Swap the palette around, and set up a table, if necessary */
         if (full_quantize != 0)
         {
            int j = num_palette;

            /* Put all the useful colors within the max, but don't
             * move the others.
             */
            for (i = 0; i < maximum_colors; i++)
            {
               if ((int)png_ptr->quantize_sort[i] >= maximum_colors)
               {
                  do
                     j--;
                  while ((int)png_ptr->quantize_sort[j] >= maximum_colors);

                  palette[i] = palette[j];
               }
            }
         }
         else
         {
            int j = num_palette;

            /* Move all the used colors inside the max limit, and
             * develop a translation table.
             */
            for (i = 0; i < maximum_colors; i++)
            {
               /* Only move the colors we need to */
               if ((int)png_ptr->quantize_sort[i] >= maximum_colors)
               {
                  png_color tmp_color;

                  do
                     j--;
                  while ((int)png_ptr->quantize_sort[j] >= maximum_colors);

                  tmp_color = palette[j];
                  palette[j] = palette[i];
                  palette[i] = tmp_color;
                  /* Indicate where the color went */
                  png_ptr->quantize_index[j] = (png_byte)i;
                  png_ptr->quantize_index[i] = (png_byte)j;
               }
            }

            /* Find closest color for those colors we are not using */
            for (i = 0; i < num_palette; i++)
            {
               if ((int)png_ptr->quantize_index[i] >= maximum_colors)
               {
                  int min_d, k, min_k, d_index;

                  /* Find the closest color to one we threw out */
                  d_index = png_ptr->quantize_index[i];
                  min_d = PNG_COLOR_DIST(palette[d_index], palette[0]);
                  for (k = 1, min_k = 0; k < maximum_colors; k++)
                  {
                     int d;

                     d = PNG_COLOR_DIST(palette[d_index], palette[k]);

                     if (d < min_d)
                     {
                        min_d = d;
                        min_k = k;
                     }
                  }
                  /* Point to closest color */
                  png_ptr->quantize_index[i] = (png_byte)min_k;
               }
            }
         }
         png_free(png_ptr, png_ptr->quantize_sort);
         png_ptr->quantize_sort = NULL;
      }
      else
      {
         /* This is much harder to do simply (and quickly).  Perhaps
          * we need to go through a median cut routine, but those
          * don't always behave themselves with only a few colors
          * as input.  So we will just find the closest two colors,
          * and throw out one of them (chosen somewhat randomly).
          * [We don't understand this at all, so if someone wants to
          *  work on improving it, be our guest - AED, GRP]
          */
         int i;
         int max_d;
         int num_new_palette;
         png_dsortp t;
         png_dsortpp hash;

         t = NULL;

         /* Initialize palette index arrays */
         png_ptr->index_to_palette = (png_bytep)png_malloc(png_ptr,
             (png_alloc_size_t)num_palette);
         png_ptr->palette_to_index = (png_bytep)png_malloc(png_ptr,
             (png_alloc_size_t)num_palette);

         /* Initialize the sort array */
         for (i = 0; i < num_palette; i++)
         {
            png_ptr->index_to_palette[i] = (png_byte)i;
            png_ptr->palette_to_index[i] = (png_byte)i;
         }

         hash = (png_dsortpp)png_calloc(png_ptr, (png_alloc_size_t)(769 *
             (sizeof (png_dsortp))));

         num_new_palette = num_palette;

         /* Initial wild guess at how far apart the farthest pixel
          * pair we will be eliminating will be.  Larger
          * numbers mean more areas will be allocated, Smaller
          * numbers run the risk of not saving enough data, and
          * having to do this all over again.
          *
          * I have not done extensive checking on this number.
          */
         max_d = 96;

         while (num_new_palette > maximum_colors)
         {
            for (i = 0; i < num_new_palette - 1; i++)
            {
               int j;

               for (j = i + 1; j < num_new_palette; j++)
               {
                  int d;

                  d = PNG_COLOR_DIST(palette[i], palette[j]);

                  if (d <= max_d)
                  {

                     t = (png_dsortp)png_malloc_warn(png_ptr,
                         (png_alloc_size_t)(sizeof (png_dsort)));

             