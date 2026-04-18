// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/bmp.c
// Segment 16/24



        offset=start_position+14+profile_data;
        if ((offset < TellBlob(image)) ||
            (SeekBlob(image,offset,SEEK_SET) != offset) ||
            (blob_size < (MagickSizeType) (offset+profile_size)))
          ThrowReaderException(CorruptImageError,"ImproperImageHeader");
        profile=AcquireProfileStringInfo("icc",(size_t) profile_size,
          exception);
        if (profile == (StringInfo *) NULL)
          (void) SeekBlob(image,profile_size,SEEK_CUR);
        else
          {
            datum=GetStringInfoDatum(profile);
            if (ReadBlob(image,(size_t) profile_size,datum) != (ssize_t) profile_size)
              profile=DestroyStringInfo(profile);
            else
              {
                MagickOffsetType
                  profile_size_orig;

                /*
                 Trimming padded bytes.
                */
                profile_size_orig=(MagickOffsetType) datum[0] << 24;
                profile_size_orig|=(MagickOffsetType) datum[1] << 16;
                profile_size_orig|=(MagickOffsetType) datum[2] << 8;
                profile_size_orig|=(MagickOffsetType) datum[3];
                if (profile_size_orig < profile_size)
                  SetStringInfoLength(profile,(size_t) profile_size_orig);
                if (image->debug != MagickFalse)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                    "Profile: ICC, %u bytes",(unsigned int) profile_size_orig);
                (void) SetImageProfilePrivate(image,profile,exception);
              }
          }
      }
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    offset=(MagickOffsetType) bmp_info.ba_offset;
    if (offset != 0)
      if ((offset < TellBlob(image)) ||
          (SeekBlob(image,offset,SEEK_SET) != offset))
        ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    *magick='\0';
    count=ReadBlob(image,2,magick);
    if ((count == 2) && (IsBMP(magick,2) != MagickFalse))
      {
        /*
          Acquire next image structure.
        */
        AcquireNextImage(image_info,image,exception);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            status=MagickFalse;
            break;
          }
        image=SyncNextImageInList(image);
        status=SetImageProgress(image,LoadImagesTag,TellBlob(image),blob_size);
        if (status == MagickFalse)
          break;
      }
  } while (IsBMP(magick,2) != MagickFalse);
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r B M P I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterBMPImage() adds attributes for the BMP image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterBMPImage method is:
%
%      size_t RegisterBMPImage(void)
%
*/
ModuleExport size_t RegisterBMPImage(void)
{
  MagickInfo
    *entry;