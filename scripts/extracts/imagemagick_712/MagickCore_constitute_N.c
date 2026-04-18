// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/constitute.c
// Segment 14/15



              GetPathComponent(image->filename,ExtensionPath,extension);
              if (*extension != '\0')
                magick_info=GetMagickInfo(extension,exception);
              else
                magick_info=GetMagickInfo(image->magick,exception);
              (void) CopyMagickString(image->filename,filename,
                MagickPathExtent);
              encoder=GetImageEncoder(magick_info);
              (void) ThrowMagickException(exception,GetMagickModule(),
                MissingDelegateWarning,"NoEncodeDelegateForThisImageFormat",
                "`%s'",write_info->magick);
            }
          if (encoder == (EncodeImageHandler *) NULL)
            {
              magick_info=GetMagickInfo(image->magick,exception);
              encoder=GetImageEncoder(magick_info);
              if (encoder == (EncodeImageHandler *) NULL)
                (void) ThrowMagickException(exception,GetMagickModule(),
                  MissingDelegateError,"NoEncodeDelegateForThisImageFormat",
                  "`%s'",write_info->magick);
            }
          if (encoder != (EncodeImageHandler *) NULL)
            {
              /*
                Call appropriate image writer based on image type.
              */
              if (GetMagickEncoderThreadSupport(magick_info) == MagickFalse)
                LockSemaphoreInfo(magick_info->semaphore);
              status=IsCoderAuthorized(magick_info->magick_module,write_info->magick,
                WritePolicyRights,exception);
              if (status != MagickFalse)
                status=encoder(write_info,image,exception);
              if (GetMagickEncoderThreadSupport(magick_info) == MagickFalse)
                UnlockSemaphoreInfo(magick_info->semaphore);
            }
        }
    }
  if (temporary != MagickFalse)
    {
      /*
        Copy temporary image file to permanent.
      */
      status=OpenBlob(write_info,image,ReadBinaryBlobMode,exception);
      if (status != MagickFalse)
        {
          (void) RelinquishUniqueFileResource(write_info->filename);
          status=ImageToFile(image,write_info->filename,exception);
        }
      if (CloseBlob(image) == MagickFalse)
        status=MagickFalse;
      (void) RelinquishUniqueFileResource(image->filename);
      (void) CopyMagickString(image->filename,write_info->filename,
        MagickPathExtent);
    }
  if ((LocaleCompare(write_info->magick,"info") != 0) &&
      (write_info->verbose != MagickFalse))
    (void) IdentifyImage(image,stdout,MagickFalse,exception);
  write_info=DestroyImageInfo(write_info);
  if (GetBlobError(image) != MagickFalse)
    ThrowWriterException(FileOpenError,"UnableToWriteFile");
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e I m a g e s                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteImages() writes an image sequence into one or more files.  While
%  WriteImage() can write an image sequence, it is limited to writing
%  the sequence into a single file using a format which supports multiple
%  frames.   WriteImages(), however, does not have this limitation, instead it
%  generates multiple output files if necessary (or when requested).  When
%  ImageInfo's adjoin flag is set to MagickFalse, the file name is expected
%  to include a printf-style formatting string for the frame number (e.g.
%  "image%02d.png").
%
%  The format of the WriteImages method is:
%
%      MagickBooleanType WriteImages(const ImageInfo *image_info,Image *images,
%        const char *filename,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o images: the image list.
%
%    o filename: the image filename.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType WriteImages(const ImageInfo *image_info,
  Image *images,const char *filename,ExceptionInfo *exception)
{
#define WriteImageTag  "Write/Image"

  ExceptionInfo
    *sans_exception;

  ImageInfo
    *write_info;

  MagickBooleanType
    proceed;

  MagickOffsetType
    progress;

  MagickProgressMonitor
    progress_monitor;

  MagickSizeType
    number_images;

  MagickStatusType
    status;

  Image
    *p;