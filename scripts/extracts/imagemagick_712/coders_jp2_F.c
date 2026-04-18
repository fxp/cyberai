// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jp2.c
// Segment 6/11



        index=comps_info[i].y_index/jp2_image->comps[i].dx+x/
          jp2_image->comps[i].dx;
        if ((index < 0) ||
            (index >= (ssize_t) (jp2_image->comps[i].h*jp2_image->comps[i].w)))
          {
            opj_destroy_codec(jp2_codec);
            opj_image_destroy(jp2_image);
            ThrowReaderException(CoderError,
              "IrregularChannelGeometryNotSupported")
          }
        pixel=comps_info[i].scale*(jp2_image->comps[i].data[index]+
          comps_info[i].addition);
        switch (i)
        {
          case 0:
          {
            if (jp2_image->numcomps == 1)
              {
                SetPixelGray(image,ClampToQuantum(pixel),q);
                if (jp2_image->comps[i].alpha != 0)
                  SetPixelAlpha(image,ClampToQuantum(pixel),q);
                break;
              }
            SetPixelRed(image,ClampToQuantum(pixel),q);
            if (jp2_image->numcomps == 2)
              {
                SetPixelGreen(image,ClampToQuantum(pixel),q);
                SetPixelBlue(image,ClampToQuantum(pixel),q);
              }
            break;
          }
          case 1:
          {
            if ((jp2_image->numcomps == 2) && (jp2_image->comps[i].alpha != 0))
              {
                SetPixelAlpha(image,ClampToQuantum(pixel),q);
                break;
              }
            SetPixelGreen(image,ClampToQuantum(pixel),q);
            break;
          }
          case 2:
          {
            SetPixelBlue(image,ClampToQuantum(pixel),q);
            break;
          }
          case 3:
          {
            if ((image->alpha_trait & BlendPixelTrait) != 0)
              SetPixelAlpha(image,ClampToQuantum(pixel),q);
            else
              SetPixelChannel(image,MetaPixelChannels,ClampToQuantum(pixel),q);
            break;
          }
          default:
          {
            ssize_t
              offset = MetaPixelChannels+i-3;

            if ((image->alpha_trait & BlendPixelTrait) != 0)
              offset--;
            SetPixelChannel(image,(PixelChannel) offset,ClampToQuantum(pixel),
              q);
            break;
          }
        }
      }
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
    status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
      image->rows);
    if (status == MagickFalse)
      break;
  }
  /*
    Free resources.
  */
  opj_destroy_codec(jp2_codec);
  opj_image_destroy(jp2_image);
  (void) CloseBlob(image);
  if ((image_info->number_scenes != 0) && (image_info->scene != 0))
    AppendImageToList(&image,CloneImage(image,0,0,MagickTrue,exception));
  return(GetFirstImageInList(image));
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r J P 2 I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterJP2Image() adds attributes for the JP2 image format to the list of
%  supported formats.  The attributes include the image format tag, a method
%  method to read and/or write the format, whether the format supports the
%  saving of more than one frame to the same file or blob, whether the format
%  supports native in-memory I/O, and a brief description of the format.
%
%  The format of the RegisterJP2Image method is:
%
%      size_t RegisterJP2Image(void)
%
*/
ModuleExport size_t RegisterJP2Image(void)
{
  char
    version[MagickPathExtent];

  MagickInfo
    *entry;