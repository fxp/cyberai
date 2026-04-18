// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 34/34



        /*
          Colormapped TIFF image.
        */
        red=(uint16 *) AcquireQuantumMemory(65536,sizeof(*red));
        green=(uint16 *) AcquireQuantumMemory(65536,sizeof(*green));
        blue=(uint16 *) AcquireQuantumMemory(65536,sizeof(*blue));
        if ((red == (uint16 *) NULL) || (green == (uint16 *) NULL) ||
            (blue == (uint16 *) NULL))
          {
            if (red != (uint16 *) NULL)
              red=(uint16 *) RelinquishMagickMemory(red);
            if (green != (uint16 *) NULL)
              green=(uint16 *) RelinquishMagickMemory(green);
            if (blue != (uint16 *) NULL)
              blue=(uint16 *) RelinquishMagickMemory(blue);
            ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
          }
        /*
          Initialize TIFF colormap.
        */
        (void) memset(red,0,65536*sizeof(*red));
        (void) memset(green,0,65536*sizeof(*green));
        (void) memset(blue,0,65536*sizeof(*blue));
        for (i=0; i < (ssize_t) image->colors; i++)
        {
          red[i]=ScaleQuantumToShort((Quantum) image->colormap[i].red);
          green[i]=ScaleQuantumToShort((Quantum) image->colormap[i].green);
          blue[i]=ScaleQuantumToShort((Quantum) image->colormap[i].blue);
        }
        (void) TIFFSetField(tiff,TIFFTAG_COLORMAP,red,green,blue);
        red=(uint16 *) RelinquishMagickMemory(red);
        green=(uint16 *) RelinquishMagickMemory(green);
        blue=(uint16 *) RelinquishMagickMemory(blue);
        magick_fallthrough;
      }
      default:
      {
        /*
          Convert PseudoClass packets to contiguous grayscale scanlines.
        */
        quantum_type=IndexQuantum;
        if (image->alpha_trait != UndefinedPixelTrait)
          {
            if (photometric != PHOTOMETRIC_PALETTE)
              quantum_type=GrayAlphaQuantum;
            else
              quantum_type=IndexAlphaQuantum;
           }
         else
           if (photometric != PHOTOMETRIC_PALETTE)
             quantum_type=GrayQuantum;
        if (image->number_meta_channels != 0)
          quantum_type=MultispectralQuantum;
        status=WriteTIFFChannels(image,tiff,tiff_info,quantum_info,
          quantum_type,0,pixels,exception);
        break;
      }
    }
    quantum_info=DestroyQuantumInfo(quantum_info);
    if (image->colorspace == LabColorspace)
      DecodeLabImage(image,exception);
    DestroyTIFFInfo(&tiff_info);
    /* TIFFPrintDirectory(tiff,stdout,MagickFalse); */
    if (status == MagickFalse)
      break;
    if (TIFFWriteDirectory(tiff) == 0)
      {
        status=MagickFalse;
        break;
      }
    image=SyncNextImageInList(image);
    if (image == (Image *) NULL)
      break;
    status=SetImageProgress(image,SaveImagesTag,scene++,number_scenes);
    if (status == MagickFalse)
      break;
  } while (adjoin != MagickFalse);
  if (TIFFFlush(tiff) != 1)
    status=MagickFalse;
  TIFFClose(tiff);
  return(status);
}
#endif
