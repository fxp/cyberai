// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 33/34



            /*
              Plane interlacing:  RRRRRR...GGGGGG...BBBBBB...
            */
            status=WriteTIFFChannels(image,tiff,tiff_info,quantum_info,
              RedQuantum,sample++,pixels,exception);
            if (status == MagickFalse)
              break;
            status=WriteTIFFChannels(image,tiff,tiff_info,quantum_info,
              GreenQuantum,sample++,pixels,exception);
            if (status == MagickFalse)
              break;
            status=WriteTIFFChannels(image,tiff,tiff_info,quantum_info,
              BlueQuantum,sample++,pixels,exception);
            if (status == MagickFalse)
              break;
            if (image->alpha_trait != UndefinedPixelTrait)
              {
                status=WriteTIFFChannels(image,tiff,tiff_info,quantum_info,
                  AlphaQuantum,sample++,pixels,exception);
                if (status == MagickFalse)
                  break;
              }
            for (i=0; i < (ssize_t) image->number_meta_channels; i++)
            {
              (void) SetQuantumMetaChannel(image,quantum_info,i);
              status=WriteTIFFChannels(image,tiff,tiff_info,quantum_info,
                MultispectralQuantum,sample++,pixels,exception);
              if (status == MagickFalse)
                break;
            }
            (void) SetQuantumMetaChannel(image,quantum_info,-1);
            break;
          }
        }
        break;
      }
      case PHOTOMETRIC_SEPARATED:
      {
        /*
          CMYK TIFF image.
        */
        quantum_type=CMYKQuantum;
        if (image->alpha_trait != UndefinedPixelTrait)
          quantum_type=CMYKAQuantum;
        if (image->number_meta_channels != 0)
          {
            quantum_type=MultispectralQuantum;
            (void) SetQuantumPad(image,quantum_info,0);
          }
        if (image->colorspace != CMYKColorspace)
          (void) TransformImageColorspace(image,CMYKColorspace,exception);
        status=WriteTIFFChannels(image,tiff,tiff_info,quantum_info,
          quantum_type,0,pixels,exception);
        break;
      }
      case PHOTOMETRIC_PALETTE:
      {
        uint16
          *blue,
          *green,
          *red;