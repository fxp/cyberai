// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 94/94



#if defined(JNG_SUPPORTED)
   if (image_info->compression == JPEGCompression)
     {
       ImageInfo
         *write_info;

       if (logging != MagickFalse)
         (void) LogMagickEvent(CoderEvent,GetMagickModule(),
           "  Writing JNG object.");
       /* To do: specify the desired alpha compression method. */
       write_info=CloneImageInfo(image_info);
       write_info->compression=UndefinedCompression;
       status=WriteOneJNGImage(mng_info,write_info,image,exception);
       write_info=DestroyImageInfo(write_info);
     }
   else
#endif
     {
       if (logging != MagickFalse)
         (void) LogMagickEvent(CoderEvent,GetMagickModule(),
           "  Writing PNG object.");

       mng_info->need_blob = MagickFalse;
       mng_info->preserve_colormap = MagickFalse;

       /* We don't want any ancillary chunks written */
       mng_info->exclude_bKGD=MagickTrue;
       mng_info->exclude_caNv=MagickTrue;
       mng_info->exclude_cHRM=MagickTrue;
       mng_info->exclude_date=MagickTrue;
       mng_info->exclude_EXIF=MagickTrue;
       mng_info->exclude_gAMA=MagickTrue;
       mng_info->exclude_iCCP=MagickTrue;
       mng_info->exclude_oFFs=MagickTrue;
       mng_info->exclude_pHYs=MagickTrue;
       mng_info->exclude_sRGB=MagickTrue;
       mng_info->exclude_tEXt=MagickTrue;
       mng_info->exclude_tRNS=MagickTrue;
       mng_info->exclude_zCCP=MagickTrue;
       mng_info->exclude_zTXt=MagickTrue;

       status=WriteOnePNGImage(mng_info,image_info,image,exception);
     }

    if (status == MagickFalse)
      {
        (void) CloseBlob(image);
        mng_info=(MngWriteInfo *) RelinquishMagickMemory(mng_info);
        return(MagickFalse);
      }
    (void) CatchImageException(image);
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    status=SetImageProgress(image,SaveImagesTag,(MagickOffsetType) scene++,
      number_scenes);

    if (status == MagickFalse)
      break;

  } while (mng_info->adjoin != MagickFalse);

  if (write_mng != MagickFalse)
    {
      /*
        Write the MEND chunk.
      */
      (void) WriteBlobMSBULong(mng_info->image,0x00000000L);
      PNGType(chunk,mng_MEND);
      LogPNGChunk(logging,mng_MEND,0L);
      (void) WriteBlob(mng_info->image,4,chunk);
      (void) WriteBlobMSBULong(mng_info->image,crc32(0,chunk,4));
    }
  /*
    Relinquish resources.
  */
  (void) CloseBlob(mng_info->image);
  mng_info=(MngWriteInfo *) RelinquishMagickMemory(mng_info);

  if (logging != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"exit WriteMNGImage()");

  return(MagickTrue);
}
#endif
