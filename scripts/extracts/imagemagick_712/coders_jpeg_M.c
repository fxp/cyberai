// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 13/24



      if ((buffer[i] != signature[j]) && (buffer[i] != alt_signature[j]))
        {
          j=0;
          continue;
        }
      if (++j != SIGNATURE_SIZE)
        continue;
      offset+=i-SIGNATURE_SIZE+1;
      old_offset=offset;
      (void) CloseBlob(image);
      jpeg_image=ReadOneJPEGImage(image_info,jpeg_info,&offset,exception);
      if (jpeg_image != (Image *) NULL)
        AppendImageToList(&images,jpeg_image);
      if (offset <= old_offset)
        break;
      status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
      if (status == MagickFalse)
        break;
      (void) SeekBlob(image,offset,SEEK_SET);
      count=0;
      j=0;
      break;
    }
    offset+=count;
  }
  (void) CloseBlob(image);
  image=DestroyImageList(image);
  return(status);
}

static Image *ReadJPEGImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  Image
    *images;

  struct jpeg_decompress_struct
    jpeg_info;

  MagickOffsetType
    offset;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  offset=0;
  images=ReadOneJPEGImage(image_info,&jpeg_info,&offset,exception);
  if ((images != (Image *) NULL) &&
      (LocaleCompare(image_info->magick,"MPO") == 0))
    {
      const StringInfo
        *profile;

      /*
        Check for multi-picture object.
      */
      profile=GetImageProfile(images,"MPF");
      if (profile != (const StringInfo *) NULL)
        (void) ReadMPOImages(image_info,&jpeg_info,images,offset,exception);
    }
  return(images);
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r J P E G I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterJPEGImage() adds properties for the JPEG image format to
%  the list of supported formats.  The properties include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterJPEGImage method is:
%
%      size_t RegisterJPEGImage(void)
%
*/
ModuleExport size_t RegisterJPEGImage(void)
{
#define JPEGDescription "Joint Photographic Experts Group JFIF format"
#define JPEGStringify(macro_or_string)  JPEGStringifyArg(macro_or_string)
#define JPEGStringifyArg(contents)  #contents

  char
    version[MagickPathExtent];

  MagickInfo
    *entry;