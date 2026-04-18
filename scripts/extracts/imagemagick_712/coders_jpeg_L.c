// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 12/24



    /*
      Uncompress one JPEG scanline.
    */
    if (jpeg_info->data_precision > 12)
      {
#if defined(MAGICKCORE_HAVE_JPEG16_READ_SCANLINES)
        number_scanlines=jpeg16_read_scanlines(jpeg_info,(J16SAMPROW *)
          &jpeg_pixels,1);
#endif
      }
    else if (jpeg_info->data_precision > 8)
      {
#if defined(MAGICKCORE_HAVE_JPEG12_READ_SCANLINES)
        number_scanlines=jpeg12_read_scanlines(jpeg_info,(J12SAMPROW *)
          &jpeg_pixels,1);
#endif
      }
    else
      {
        number_scanlines=jpeg_read_scanlines(jpeg_info,(JSAMPROW *)
          &jpeg_pixels,1);
      }
    if (number_scanlines != 1)
      (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
        "AnErrorHasOccurredReadingFromFile","`%s'",image->filename);
    /*
      Transfer image pixels from JPEG buffer.
    */
    q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    p=jpeg_pixels;
    switch (jpeg_info->output_components)
    {
      case 1:
      {
        /*
          Grayscale.
        */
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          Quantum
            index;

          unsigned short
            pixel;

          if (jpeg_info->data_precision > 8)
            pixel=(*(unsigned short *) p);
          else
            pixel=(*(JSAMPLE *) p);

          index=(Quantum) ConstrainColormapIndex(image,pixel,exception);
          SetPixelViaPixelInfo(image,image->colormap+(ssize_t) index,q);
          SetPixelIndex(image,index,q);
          p+=(ptrdiff_t) bytes_per_pixel;
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        break;
      }
      case 4:
      {
        /*
          CMYK.
        */
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          SetPixelCyan(image,QuantumRange-JPEGGetQuantum(jpeg_info,range,p),q);
          p+=(ptrdiff_t) bytes_per_pixel;
          SetPixelMagenta(image,QuantumRange-JPEGGetQuantum(jpeg_info,range,p),q);
          p+=(ptrdiff_t) bytes_per_pixel;
          SetPixelYellow(image,QuantumRange-JPEGGetQuantum(jpeg_info,range,p),q);
          p+=(ptrdiff_t) bytes_per_pixel;
          SetPixelBlack(image,QuantumRange-JPEGGetQuantum(jpeg_info,range,p),q);
          p+=(ptrdiff_t) bytes_per_pixel;
          SetPixelAlpha(image,OpaqueAlpha,q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        break;
      }
      default:
      {
        /*
          RGB || YCC.
        */
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          SetPixelRed(image,JPEGGetQuantum(jpeg_info,range,p),q);
          p+=(ptrdiff_t) bytes_per_pixel;
          SetPixelGreen(image,JPEGGetQuantum(jpeg_info,range,p),q);
          p+=(ptrdiff_t) bytes_per_pixel;
          SetPixelBlue(image,JPEGGetQuantum(jpeg_info,range,p),q);
          p+=(ptrdiff_t) bytes_per_pixel;
          SetPixelAlpha(image,OpaqueAlpha,q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        break;
      }
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
    status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
      image->rows);
    if (status == MagickFalse)
      {
        jpeg_abort_decompress(jpeg_info);
        break;
      }
  }
  if (status != MagickFalse)
    {
      client_info->finished=MagickTrue;
      if (setjmp(client_info->error_recovery) == 0)
        (void) jpeg_finish_decompress(jpeg_info);
    }
  /*
    Free jpeg resources.
  */
  client_info=JPEGCleanup(jpeg_info,client_info);
  memory_info=RelinquishVirtualMemory(memory_info);
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}

static MagickBooleanType ReadMPOImages(const ImageInfo *image_info,
  struct jpeg_decompress_struct *jpeg_info,Image *images,
  const MagickOffsetType start_offset,ExceptionInfo *exception)
{
#define BUFFER_SIZE  8192
#define SIGNATURE_SIZE 4

  Image
    *image;

  MagickBooleanType
    status;

  MagickOffsetType
    offset = start_offset;

  ssize_t
    count,
    j = 0;

  unsigned char
    alt_signature[SIGNATURE_SIZE] = {0xff, 0xd8, 0xff, 0xe1},
    buffer[BUFFER_SIZE],
    signature[SIGNATURE_SIZE] = {0xff, 0xd8, 0xff, 0xe0};

  /*
    Read multi-picture object images.
  */
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return(MagickFalse);
    }
  (void) SeekBlob(image,offset,SEEK_SET);
  while ((count=ReadBlob(image,BUFFER_SIZE,buffer)) != 0)
  {
    ssize_t
      i;

    for (i=0; i < count; i++)
    {
      Image
        *jpeg_image;

      MagickOffsetType
        old_offset;