// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 6/24



static void JPEGSetImageQuality(const struct jpeg_decompress_struct *jpeg_info,
  Image *image)
{
  image->quality=UndefinedCompressionQuality;
#if defined(D_PROGRESSIVE_SUPPORTED)
  if (image->compression == LosslessJPEGCompression)
    {
      image->quality=100;
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "Quality: 100 (lossless)");
    }
  else
#endif
  {
    ssize_t
      i,
      j,
      qvalue,
      sum;

    /*
      Determine the JPEG compression quality from the quantization tables.
    */
    sum=0;
    for (i=0; i < NUM_QUANT_TBLS; i++)
    {
      if (jpeg_info->quant_tbl_ptrs[i] != NULL)
        for (j=0; j < DCTSIZE2; j++)
          sum+=jpeg_info->quant_tbl_ptrs[i]->quantval[j];
    }
    if ((jpeg_info->quant_tbl_ptrs[0] != NULL) &&
        (jpeg_info->quant_tbl_ptrs[1] != NULL))
      {
        ssize_t
          hash[101] =
          {
            1020, 1015,  932,  848,  780,  735,  702,  679,  660,  645,
             632,  623,  613,  607,  600,  594,  589,  585,  581,  571,
             555,  542,  529,  514,  494,  474,  457,  439,  424,  410,
             397,  386,  373,  364,  351,  341,  334,  324,  317,  309,
             299,  294,  287,  279,  274,  267,  262,  257,  251,  247,
             243,  237,  232,  227,  222,  217,  213,  207,  202,  198,
             192,  188,  183,  177,  173,  168,  163,  157,  153,  148,
             143,  139,  132,  128,  125,  119,  115,  108,  104,   99,
              94,   90,   84,   79,   74,   70,   64,   59,   55,   49,
              45,   40,   34,   30,   25,   20,   15,   11,    6,    4,
               0
          },
          sums[101] =
          {
            32640, 32635, 32266, 31495, 30665, 29804, 29146, 28599, 28104,
            27670, 27225, 26725, 26210, 25716, 25240, 24789, 24373, 23946,
            23572, 22846, 21801, 20842, 19949, 19121, 18386, 17651, 16998,
            16349, 15800, 15247, 14783, 14321, 13859, 13535, 13081, 12702,
            12423, 12056, 11779, 11513, 11135, 10955, 10676, 10392, 10208,
             9928,  9747,  9564,  9369,  9193,  9017,  8822,  8639,  8458,
             8270,  8084,  7896,  7710,  7527,  7347,  7156,  6977,  6788,
             6607,  6422,  6236,  6054,  5867,  5684,  5495,  5305,  5128,
             4945,  4751,  4638,  4442,  4248,  4065,  3888,  3698,  3509,
             3326,  3139,  2957,  2775,  2586,  2405,  2216,  2037,  1846,
             1666,  1483,  1297,  1109,   927,   735,   554,   375,   201,
              128,     0
          };