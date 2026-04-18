// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/constitute.c
// Segment 8/15



            lsb_first=1;
            read_info->endian=(*(char *) &lsb_first) == 1 ? LSBEndian :
              MSBEndian;
         }
    }
  if ((magick_info != (const MagickInfo *) NULL) &&
      (GetMagickDecoderSeekableStream(magick_info) != MagickFalse))
    {
      image=AcquireImage(read_info,exception);
      (void) CopyMagickString(image->filename,read_info->filename,
        MagickPathExtent);
      status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
      if (status == MagickFalse)
        {
          read_info=DestroyImageInfo(read_info);
          image=DestroyImage(image);
          return((Image *) NULL);
        }
      if (IsBlobSeekable(image) == MagickFalse)
        {
          /*
            Coder requires a seekable stream.
          */
          *read_info->filename='\0';
          status=ImageToFile(image,read_info->filename,exception);
          if (status == MagickFalse)
            {
              (void) CloseBlob(image);
              read_info=DestroyImageInfo(read_info);
              image=DestroyImage(image);
              return((Image *) NULL);
            }
          read_info->temporary=MagickTrue;
        }
      (void) CloseBlob(image);
      image=DestroyImage(image);
    }
  image=NewImageList();
  decoder=GetImageDecoder(magick_info);
  if (decoder == (DecodeImageHandler *) NULL)
    {
      delegate_info=GetDelegateInfo(read_info->magick,(char *) NULL,exception);
      if (delegate_info == (const DelegateInfo *) NULL)
        {
          (void) SetImageInfo(read_info,0,exception);
          (void) CopyMagickString(read_info->filename,filename,
            MagickPathExtent);
          magick_info=GetMagickInfo(read_info->magick,exception);
          decoder=GetImageDecoder(magick_info);
        }
    }
  if (decoder != (DecodeImageHandler *) NULL)
    {
      /*
        Call appropriate image reader based on image type.
      */
      if (GetMagickDecoderThreadSupport(magick_info) == MagickFalse)
        LockSemaphoreInfo(magick_info->semaphore);
      status=IsCoderAuthorized(magick_info->magick_module,read_info->magick,
        ReadPolicyRights,exception);
      image=(Image *) NULL;
      if (status != MagickFalse)
        image=decoder(read_info,exception);
      if (GetMagickDecoderThreadSupport(magick_info) == MagickFalse)
        UnlockSemaphoreInfo(magick_info->semaphore);
    }
  else
    {
      delegate_info=GetDelegateInfo(read_info->magick,(char *) NULL,exception);
      if (delegate_info == (const DelegateInfo *) NULL)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),
            MissingDelegateError,"NoDecodeDelegateForThisImageFormat","`%s'",
            read_info->filename);
          if (read_info->temporary != MagickFalse)
            (void) RelinquishUniqueFileResource(read_info->filename);
          read_info=DestroyImageInfo(read_info);
          return((Image *) NULL);
        }
      /*
        Let our decoding delegate process the image.
      */
      image=AcquireImage(read_info,exception);
      if (image == (Image *) NULL)
        {
          read_info=DestroyImageInfo(read_info);
          return((Image *) NULL);
        }
      (void) CopyMagickString(image->filename,read_info->filename,
        MagickPathExtent);
      *read_info->filename='\0';
      if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
        LockSemaphoreInfo(delegate_info->semaphore);
      status=InvokeDelegate(read_info,image,read_info->magick,(char *) NULL,
        exception);
      if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
        UnlockSemaphoreInfo(delegate_info->semaphore);
      image=DestroyImageList(image);
      read_info->temporary=MagickTrue;
      if (status != MagickFalse)
        (void) SetImageInfo(read_info,0,exception);
      magick_info=GetMagickInfo(read_info->magick,exception);
      decoder=GetImageDecoder(magick_info);
      if (decoder == (DecodeImageHandler *) NULL)
        {
          if (IsPathAccessible(read_info->filename) != MagickFalse)
            (void) ThrowMagickException(exception,GetMagickModule(),
              MissingDelegateError,"NoDecodeDelegateForThisImageFormat","`%s'",
              read_info->magick);
          else
            ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
              read_info->filename);
          read_info=DestroyImageInfo(read_info);
          return((Image *) NULL);
        }
      /*
        Call appropriate image reader based on image type.
      */
      if (GetMagickDecoderThreadSupport(magick_info) == MagickFalse)
        LockSemaphoreInfo(magick_info->semaphore);
      status=IsCoderAuthorized(magick_info->magick_module,read_info->magick,
        ReadPolicyRights,exception);
      image=(Image *) NULL;
      if (status != MagickFalse)
        image=(decoder)(read_info,exception);
      if (GetMagickDecoderThreadSupport(magick_info) == MagickFalse)
        UnlockSemaphoreInfo(magick_info->semaphore);
    }
  if