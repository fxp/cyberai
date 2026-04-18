// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/webp.c
// Segment 4/11



    mux=WebPMuxCreate(&data,0);
    status=WebPMuxGetAnimationParams(mux,&params);
    if (status >= 0)
      image->iterations=(size_t) params.loop_count;
    WebPMuxDelete(mux);
  }
  demux=WebPDemux(&data);
  if (WebPDemuxGetFrame(demux,1,&iter))
    {
      do
      {
        if (image_count != 0)
          {
            AcquireNextImage(image_info,image,exception);
            if (GetNextImageInList(image) == (Image *) NULL)
              break;
            image=SyncNextImageInList(image);
            CloneImageProperties(image,original_image);
            image->page.x=(ssize_t) iter.x_offset;
            image->page.y=(ssize_t) iter.y_offset;
            webp_status=ReadSingleWEBPImage(image_info,image,
              iter.fragment.bytes,iter.fragment.size,configure,exception,
              MagickFalse);
          }
        else
          {
            image->page.x=(ssize_t) iter.x_offset;
            image->page.y=(ssize_t) iter.y_offset;
            webp_status=ReadSingleWEBPImage(image_info,image,
              iter.fragment.bytes,iter.fragment.size,configure,exception,
              MagickTrue);
          }
        if (webp_status != VP8_STATUS_OK)
          break;
        image->page.width=canvas_width;
        image->page.height=canvas_height;
        image->ticks_per_second=100;
        image->delay=(size_t) round(iter.duration/10.0);
        image->dispose=NoneDispose;
        if (iter.dispose_method == WEBP_MUX_DISPOSE_BACKGROUND)
          image->dispose=BackgroundDispose;
        (void) SetImageProperty(image,"webp:mux-blend",
          "AtopPreviousAlphaBlend",exception);
        if (iter.blend_method == WEBP_MUX_BLEND)
          (void) SetImageProperty(image,"webp:mux-blend",
            "AtopBackgroundAlphaBlend",exception);
        image_count++;
      } while (WebPDemuxNextFrame(&iter));
      WebPDemuxReleaseIterator(&iter);
    }
  WebPDemuxDelete(demux);
  return(webp_status);
}
#endif

static Image *ReadWEBPImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
#define ThrowWEBPException(severity,tag) \
{ \
  if (stream != (unsigned char *) NULL) \
    stream=(unsigned char*) RelinquishMagickMemory(stream); \
  if (webp_image != (WebPDecBuffer *) NULL) \
    WebPFreeDecBuffer(webp_image); \
  ThrowReaderException(severity,tag); \
}

  Image
    *image;

  int
    webp_status;

  MagickBooleanType
    status;

  size_t
    blob_size,
    length;

  ssize_t
    count;

  unsigned char
    header[12],
    *stream;

  WebPDecoderConfig
    configure;

  WebPDecBuffer
    *magick_restrict webp_image = &configure.output;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  stream=(unsigned char *) NULL;
  if (WebPInitDecoderConfig(&configure) == 0)
    ThrowReaderException(ResourceLimitError,"UnableToDecodeImageFile");
  webp_image->colorspace=MODE_RGBA;
  count=ReadBlob(image,12,header);
  if (count != 12)
    ThrowWEBPException(CorruptImageError,"InsufficientImageDataInFile");
  status=IsWEBP(header,(size_t) count);
  if (status == MagickFalse)
    ThrowWEBPException(CorruptImageError,"CorruptImage");
  length=(size_t) (ReadWebPLSBWord(header+4)+8);
  if (length < 12)
    ThrowWEBPException(CorruptImageError,"CorruptImage");
  blob_size=(size_t) GetBlobSize(image);
  if (length > blob_size)
    {
      size_t
        delta=length-blob_size;

      if (delta != 12 && delta != (12 + 8))
        ThrowWEBPException(CorruptImageError,"InsufficientImageDataInFile");
      length-=delta;
      WriteWebPLSBWord(header+4,length-8);
    }
  stream=(unsigned char *) AcquireQuantumMemory(length,sizeof(*stream));
  if (stream == (unsigned char *) NULL)
    ThrowWEBPException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memcpy(stream,header,12);
  count=ReadBlob(image,length-12,stream+12);
  if (count != (ssize_t) (length-12))
    ThrowWEBPException(CorruptImageError,"InsufficientImageDataInFile");
  webp_status=FillBasicWEBPInfo(image,stream,length,&configure);
  if (webp_status == VP8_STATUS_OK) {
    if (configure.input.has_animation) {
#if defined(MAGICKCORE_WEBPMUX_DELEGATE)
      webp_status=ReadAnimatedWEBPImage(image_info,image,stream,length,
        &configure,exception);
#else
      webp_status=VP8_STATUS_UNSUPPORTED_FEATURE;
#endif
    } else {
      webp_status=ReadSingleWEBPImage(image_info,image,stream,length,
        &configure,exception,MagickFalse);
    }
  }