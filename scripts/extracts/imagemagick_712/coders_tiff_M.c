// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 13/34



        /*
          Initialize colormap.
        */
        tiff_status=TIFFGetField(tiff,TIFFTAG_COLORMAP,&red_colormap,
          &green_colormap,&blue_colormap);
        if (tiff_status == 1)
          {
            if ((red_colormap != (uint16 *) NULL) &&
                (green_colormap != (uint16 *) NULL) &&
                (blue_colormap != (uint16 *) NULL))
              {
                range=255;  /* might be old style 8-bit colormap */
                for (i=0; i < (ssize_t) image->colors; i++)
                  if ((red_colormap[i] >= 256) || (green_colormap[i] >= 256) ||
                      (blue_colormap[i] >= 256))
                    {
                      range=65535;
                      break;
                    }
                for (i=0; i < (ssize_t) image->colors; i++)
                {
                  image->colormap[i].red=ClampToQuantum(((double)
                    QuantumRange*red_colormap[i])/range);
                  image->colormap[i].green=ClampToQuantum(((double)
                    QuantumRange*green_colormap[i])/range);
                  image->colormap[i].blue=ClampToQuantum(((double)
                    QuantumRange*blue_colormap[i])/range);
                }
              }
          }
      }
    if (image_info->ping != MagickFalse)
      {
        if (image_info->number_scenes != 0)
          if (image->scene >= (image_info->scene+image_info->number_scenes-1))
            break;
        goto next_tiff_frame;
      }
    status=SetImageExtent(image,image->columns,image->rows,exception);
    if (status == MagickFalse)
      {
        TIFFClose(tiff);
        return(DestroyImageList(image));
      }
    status=SetImageColorspace(image,image->colorspace,exception);
    status&=(MagickStatusType) ResetImagePixels(image,exception);
    if (status == MagickFalse)
      {
        TIFFClose(tiff);
        return(DestroyImageList(image));
      }
    /*
      Allocate memory for the image and pixel buffer.
    */
    quantum_info=AcquireQuantumInfo(image_info,image);
    if (quantum_info == (QuantumInfo *) NULL)
      ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
    if (sample_format == SAMPLEFORMAT_UINT)
      status=SetQuantumFormat(image,quantum_info,UnsignedQuantumFormat);
    if (sample_format == SAMPLEFORMAT_INT)
      status=SetQuantumFormat(image,quantum_info,SignedQuantumFormat);
    if (sample_format == SAMPLEFORMAT_IEEEFP)
      status=SetQuantumFormat(image,quantum_info,FloatingPointQuantumFormat);
    if (status == MagickFalse)
      ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
    status=MagickTrue;
    switch (photometric)
    {
      case PHOTOMETRIC_MINISBLACK:
      {
        SetQuantumMinIsWhite(quantum_info,MagickFalse);
        break;
      }
      case PHOTOMETRIC_MINISWHITE:
      {
        SetQuantumMinIsWhite(quantum_info,MagickTrue);
        break;
      }
      default:
        break;
    }
    tiff_status=TIFFGetFieldDefaulted(tiff,TIFFTAG_EXTRASAMPLES,&extra_samples,
      &sample_info,sans);
    if (extra_samples == 0)
      {
        if ((samples_per_pixel == 4) && (photometric == PHOTOMETRIC_RGB))
          image->alpha_trait=BlendPixelTrait;
      }
    else
      {
        for (i=0; i < extra_samples; i++)
        {
          if (sample_info[i] == EXTRASAMPLE_ASSOCALPHA)
            {
              image->alpha_trait=BlendPixelTrait;
              SetQuantumAlphaType(quantum_info,AssociatedQuantumAlpha);
              (void) SetImageProperty(image,"tiff:alpha","associated",
                exception);
              break;
            }
          else
            if (sample_info[i] == EXTRASAMPLE_UNASSALPHA)
              {
                image->alpha_trait=BlendPixelTrait;
                SetQuantumAlphaType(quantum_info,DisassociatedQuantumAlpha);
                (void) SetImageProperty(image,"tiff:alpha","unassociated",
                  exception);
                break;
              }
        }
        if ((image->alpha_trait == UndefinedPixelTrait) && (extra_samples >= 1))
          {
            const char
              *option;