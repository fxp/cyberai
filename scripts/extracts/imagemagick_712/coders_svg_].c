// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/svg.c
// Segment 29/39



  if ((fabs(affine->tx) < MagickEpsilon) && (fabs(affine->ty) < MagickEpsilon))
    {
      if ((fabs(affine->rx) < MagickEpsilon) &&
          (fabs(affine->ry) < MagickEpsilon))
        {
          if ((fabs(affine->sx-1.0) < MagickEpsilon) &&
              (fabs(affine->sy-1.0) < MagickEpsilon))
            {
              (void) WriteBlobString(image,"\">\n");
              return;
            }
          (void) FormatLocaleString(transform,MagickPathExtent,
            "\" transform=\"scale(%g,%g)\">\n",affine->sx,affine->sy);
          (void) WriteBlobString(image,transform);
          return;
        }
      else
        {
          if ((fabs(affine->sx-affine->sy) < MagickEpsilon) &&
              (fabs(affine->rx+affine->ry) < MagickEpsilon) &&
              (fabs(affine->sx*affine->sx+affine->rx*affine->rx-1.0) <
               2*MagickEpsilon))
            {
              double
                theta;

              theta=(180.0/MagickPI)*atan2(affine->rx,affine->sx);
              (void) FormatLocaleString(transform,MagickPathExtent,
                "\" transform=\"rotate(%g)\">\n",theta);
              (void) WriteBlobString(image,transform);
              return;
            }
        }
    }
  else
    {
      if ((fabs(affine->sx-1.0) < MagickEpsilon) &&
          (fabs(affine->rx) < MagickEpsilon) &&
          (fabs(affine->ry) < MagickEpsilon) &&
          (fabs(affine->sy-1.0) < MagickEpsilon))
        {
          (void) FormatLocaleString(transform,MagickPathExtent,
            "\" transform=\"translate(%g,%g)\">\n",affine->tx,affine->ty);
          (void) WriteBlobString(image,transform);
          return;
        }
    }
  (void) FormatLocaleString(transform,MagickPathExtent,
    "\" transform=\"matrix(%g %g %g %g %g %g)\">\n",
    affine->sx,affine->rx,affine->ry,affine->sy,affine->tx,affine->ty);
  (void) WriteBlobString(image,transform);
}

static MagickBooleanType IsPoint(const char *point)
{
  char
    *p;

  ssize_t
    value;

  value=(ssize_t) strtol(point,&p,10);
  (void) value;
  return(p != point ? MagickTrue : MagickFalse);
}

static MagickBooleanType TraceSVGImage(Image *image,ExceptionInfo *exception)
{
  MagickBooleanType
    status = MagickTrue;

#if defined(MAGICKCORE_AUTOTRACE_DELEGATE)
  {
    at_bitmap
      *trace;

    at_fitting_opts_type
      *fitting_options;

    at_output_opts_type
      *output_options;

    at_splines_type
      *splines;

    const Quantum
      *p;

    size_t
      number_planes;

    ssize_t
      i,
      x,
      y;

    /*
      Trace image and write as SVG.
    */
    fitting_options=at_fitting_opts_new();
    output_options=at_output_opts_new();
    number_planes=3;
    if (IdentifyImageCoderGray(image,exception) != MagickFalse)
      number_planes=1;
    trace=at_bitmap_new(image->columns,image->rows,number_planes);
    i=0;
    for (y=0; y < (ssize_t) image->rows; y++)
    {
      p=GetVirtualPixels(image,0,y,image->columns,1,exception);
      if (p == (const Quantum *) NULL)
        break;
      for (x=0; x < (ssize_t) image->columns; x++)
      {
        trace->bitmap[i++]=GetPixelRed(image,p);
        if (number_planes == 3)
          {
            trace->bitmap[i++]=GetPixelGreen(image,p);
            trace->bitmap[i++]=GetPixelBlue(image,p);
          }
        p+=(ptrdiff_t) GetPixelChannels(image);
      }
    }
    splines=at_splines_new_full(trace,fitting_options,NULL,NULL,NULL,NULL,NULL,
      NULL);
    at_splines_write(at_output_get_handler_by_suffix((char *) "svg"),
      GetBlobFileHandle(image),image->filename,output_options,splines,NULL,
      NULL);
    /*
      Free resources.
    */
    at_splines_free(splines);
    at_bitmap_free(trace);
    at_output_opts_free(output_options);
    at_fitting_opts_free(fitting_options);
  }
#else
  {
    char
      *base64,
      filename[MagickPathExtent],
      message[MagickPathExtent],
      *p;

    const DelegateInfo
      *delegate_info;

    Image
      *clone_image;

    ImageInfo
      *image_info;

    size_t
      blob_length,
      encode_length;

    ssize_t
      i;

    unsigned char
      *blob;