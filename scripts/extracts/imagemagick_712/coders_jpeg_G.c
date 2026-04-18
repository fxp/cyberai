// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 7/24



        qvalue=(ssize_t) (jpeg_info->quant_tbl_ptrs[0]->quantval[2]+
          jpeg_info->quant_tbl_ptrs[0]->quantval[53]+
          jpeg_info->quant_tbl_ptrs[1]->quantval[0]+
          jpeg_info->quant_tbl_ptrs[1]->quantval[DCTSIZE2-1]);
        for (i=0; i < 100; i++)
        {
          if ((qvalue < hash[i]) && (sum < sums[i]))
            continue;
          if (((qvalue <= hash[i]) && (sum <= sums[i])) || (i >= 50))
            image->quality=(size_t) i+1;
          if (image->debug != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "Quality: %.20g (%s)",(double) i+1,(qvalue <= hash[i]) &&
              (sum <= sums[i]) ? "exact" : "approximate");
          break;
        }
      }
    else
      if (jpeg_info->quant_tbl_ptrs[0] != NULL)
        {
          ssize_t
            hash[101] =
            {
              510,  505,  422,  380,  355,  338,  326,  318,  311,  305,
              300,  297,  293,  291,  288,  286,  284,  283,  281,  280,
              279,  278,  277,  273,  262,  251,  243,  233,  225,  218,
              211,  205,  198,  193,  186,  181,  177,  172,  168,  164,
              158,  156,  152,  148,  145,  142,  139,  136,  133,  131,
              129,  126,  123,  120,  118,  115,  113,  110,  107,  105,
              102,  100,   97,   94,   92,   89,   87,   83,   81,   79,
               76,   74,   70,   68,   66,   63,   61,   57,   55,   52,
               50,   48,   44,   42,   39,   37,   34,   31,   29,   26,
               24,   21,   18,   16,   13,   11,    8,    6,    3,    2,
                0
            },
            sums[101] =
            {
              16320, 16315, 15946, 15277, 14655, 14073, 13623, 13230, 12859,
              12560, 12240, 11861, 11456, 11081, 10714, 10360, 10027,  9679,
               9368,  9056,  8680,  8331,  7995,  7668,  7376,  7084,  6823,
               6562,  6345,  6125,  5939,  5756,  5571,  5421,  5240,  5086,
               4976,  4829,  4719,  4616,  4463,  4393,  4280,  4166,  4092,
               3980,  3909,  3835,  3755,  3688,  3621,  3541,  3467,  3396,
               3323,  3247,  3170,  3096,  3021,  2952,  2874,  2804,  2727,
               2657,  2583,  2509,  2437,  2362,  2290,  2211,  2136,  2068,
               1996,  1915,  1858,  1773,  1692,  1620,  1552,  1477,  1398,
               1326,  1251,  1179,  1109,  1031,   961,   884,   814,   736,
                667,   592,   518,   441,   369,   292,   221,   151,    86,
                 64,     0
            };

          qvalue=(ssize_t) (jpeg_info->quant_tbl_ptrs[0]->quantval[2]+
            jpeg_info->quant_tbl_ptrs[0]->quantval[53]);
          for (i=0; i < 100; i++)
          {
            if ((qvalue < hash[i]) && (sum < sums[i]))
              continue;
            if (((qvalue <= hash[i]) && (sum <= sums[i])) || (i >= 50))
              image->quality=(size_t) i+1;
            if (image->debug != MagickFalse)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "Quality: %.20g (%s)",(double) i+1,(qvalue <= hash[i]) &&
                (sum <= sums[i]) ? "exact" : "approximate");
            break;
          }
        }
  }
}

static void JPEGSetImageSamplingFactor(const struct jpeg_decompress_struct *jpeg_info,
  Image *image,ExceptionInfo *exception)
{
  char
    sampling_factor[MagickPathExtent];