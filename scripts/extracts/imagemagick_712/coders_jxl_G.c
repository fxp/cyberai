// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jxl.c
// Segment 7/11



        (void) JxlDecoderReleaseBoxBuffer(jxl_info);
        jxl_status=JxlDecoderGetBoxType(jxl_info,type,JXL_FALSE);
        if (jxl_status != JXL_DEC_SUCCESS)
          break;
        jxl_status=JxlDecoderGetBoxSizeRaw(jxl_info,&size);
        if ((jxl_status != JXL_DEC_SUCCESS) || (size <= 8))
          break;
        size-=8;
        if (LocaleNCompare(type,"Exif",sizeof(type)) == 0)
          {
            /*
              Read Exif profile.
            */
          if (exif_profile == (StringInfo *) NULL)
            {
              exif_profile=AcquireProfileStringInfo("exif",(size_t) size,
                exception);
              if (exif_profile != (StringInfo *) NULL)
                jxl_status=JxlDecoderSetBoxBuffer(jxl_info,
                  GetStringInfoDatum(exif_profile),(size_t) size);
            }
          }
        if (LocaleNCompare(type,"xml ",sizeof(type)) == 0)
          {
            /*
              Read XMP profile.
            */
            if (xmp_profile == (StringInfo *) NULL)
              {
                xmp_profile=AcquireProfileStringInfo("xmp",(size_t) size,
                  exception);
                if (xmp_profile != (StringInfo *) NULL)
                  jxl_status=JxlDecoderSetBoxBuffer(jxl_info,
                    GetStringInfoDatum(xmp_profile),(size_t) size);
              }
          }
        if (jxl_status == JXL_DEC_SUCCESS)
          jxl_status=JXL_DEC_BOX;
        break;
      }
      case JXL_DEC_SUCCESS:
      case JXL_DEC_ERROR:
        break;
      default:
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          CorruptImageError,"Unsupported status type","`%d'",jxl_status);
        jxl_status=JXL_DEC_ERROR;
        break;
      }
    }
  }
  (void) JxlDecoderReleaseBoxBuffer(jxl_info);
  JXLAddProfilesToImage(image,&exif_profile,&xmp_profile,exception);
  output_buffer=(unsigned char *) RelinquishMagickMemory(output_buffer);
  pixels=(unsigned char *) RelinquishMagickMemory(pixels);
  if (runner != NULL)
    JxlThreadParallelRunnerDestroy(runner);
  JxlDecoderDestroy(jxl_info);
  if (jxl_status == JXL_DEC_ERROR)
    ThrowReaderException(CorruptImageError,"UnableToReadImageData");
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r J X L I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterJXLImage() adds properties for the JXL image format to
%  the list of supported formats.  The properties include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterJXLImage method is:
%
%      size_t RegisterJXLImage(void)
%
*/
ModuleExport size_t RegisterJXLImage(void)
{
  char
    version[MagickPathExtent];

  MagickInfo
    *entry;

  *version='\0';
#if defined(MAGICKCORE_JXL_DELEGATE)
  (void) FormatLocaleString(version,MagickPathExtent,"libjxl %u.%u.%u",
    (JxlDecoderVersion()/1000000),(JxlDecoderVersion()/1000) % 1000,
    (JxlDecoderVersion() % 1000));
#endif
  entry=AcquireMagickInfo("JXL", "JXL", "JPEG XL (ISO/IEC 18181)");
#if defined(MAGICKCORE_JXL_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadJXLImage;
  entry->encoder=(EncodeImageHandler *) WriteJXLImage;
  entry->magick=(IsImageFormatHandler *) IsJXL;
#endif
  entry->mime_type=ConstantString("image/jxl");
  if (*version != '\0')
    entry->version=ConstantString(version);
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}