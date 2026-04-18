// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/constitute.c
// Segment 13/15



            lsb_first=1;
            image->endian=(*(char *) &lsb_first) == 1 ? LSBEndian : MSBEndian;
         }
    }
  if (SyncImagePixelCache(image,exception) == MagickFalse)
    {
      write_info=DestroyImageInfo(write_info);
      return(MagickFalse);
    }
  SyncImageProfiles(image);
  DisassociateImageStream(image);
  option=GetImageOption(image_info,"delegate:bimodal");
  if ((IsStringTrue(option) != MagickFalse) &&
      (write_info->page == (char *) NULL) &&
      (GetPreviousImageInList(image) == (Image *) NULL) &&
      (GetNextImageInList(image) == (Image *) NULL) &&
      (IsTaintImage(image) == MagickFalse) )
    {
      delegate_info=GetDelegateInfo(image->magick,write_info->magick,exception);
      if ((delegate_info != (const DelegateInfo *) NULL) &&
          (GetDelegateMode(delegate_info) == 0) &&
          (IsPathAccessible(image->magick_filename) != MagickFalse))
        {
          /*
            Process image with bi-modal delegate.
          */
          (void) CopyMagickString(image->filename,image->magick_filename,
            MagickPathExtent);
          status=InvokeDelegate(write_info,image,image->magick,
            write_info->magick,exception);
          write_info=DestroyImageInfo(write_info);
          (void) CopyMagickString(image->filename,filename,MagickPathExtent);
          return(status);
        }
    }
  status=MagickFalse;
  temporary=MagickFalse;
  if ((magick_info != (const MagickInfo *) NULL) &&
      (GetMagickEncoderSeekableStream(magick_info) != MagickFalse))
    {
      char
        image_filename[MagickPathExtent];

      (void) CopyMagickString(image_filename,image->filename,MagickPathExtent);
      status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
      (void) CopyMagickString(image->filename, image_filename,MagickPathExtent);
      if (status != MagickFalse)
        {
          if (IsBlobSeekable(image) == MagickFalse)
            {
              /*
                A seekable stream is required by the encoder.
              */
              write_info->adjoin=MagickTrue;
              (void) CopyMagickString(write_info->filename,image->filename,
                MagickPathExtent);
              (void) AcquireUniqueFilename(image->filename);
              temporary=MagickTrue;
            }
          if (CloseBlob(image) == MagickFalse)
            status=MagickFalse;
        }
    }
  encoder=GetImageEncoder(magick_info);
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
  else
    {
      delegate_info=GetDelegateInfo((char *) NULL,write_info->magick,exception);
      if (delegate_info != (DelegateInfo *) NULL)
        {
          /*
            Process the image with delegate.
          */
          *write_info->filename='\0';
          if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
            LockSemaphoreInfo(delegate_info->semaphore);
          status=InvokeDelegate(write_info,image,(char *) NULL,
            write_info->magick,exception);
          if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
            UnlockSemaphoreInfo(delegate_info->semaphore);
          (void) CopyMagickString(image->filename,filename,MagickPathExtent);
        }
      else
        {
          sans_exception=AcquireExceptionInfo();
          magick_info=GetMagickInfo(write_info->magick,sans_exception);
          if (sans_exception->severity == PolicyError)
            magick_info=GetMagickInfo(write_info->magick,exception);
          sans_exception=DestroyExceptionInfo(sans_exception);
          if ((write_info->affirm == MagickFalse) &&
              (magick_info == (const MagickInfo *) NULL))
            {
              (void) CopyMagickString(write_info->magick,image->magick,
                MagickPathExtent);
              magick_info=GetMagickInfo(write_info->magick,exception);
            }
          encoder=GetImageEncoder(magick_info);
          if (encoder == (EncodeImageHandler *) NULL)
            {
              char
                extension[MagickPathExtent];