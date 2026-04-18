// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/webp.c
// Segment 3/11



  webp_image=&configure->output;
  if (is_first != MagickFalse)
    {
      canvas_width=image->columns;
      canvas_height=image->rows;
      x_offset=image->page.x;
      y_offset=image->page.y;
      image->page.x=0;
      image->page.y=0;
    }
  else
    {
      canvas_width=0;
      canvas_height=0;
      x_offset=0;
      y_offset=0;
    }
  webp_status=FillBasicWEBPInfo(image,stream,length,configure);
  image_width=image->columns;
  image_height=image->rows;
  if (is_first)
    {
      image->columns=canvas_width;
      image->rows=canvas_height;
    }
  if (webp_status != VP8_STATUS_OK)
    return(webp_status);
  if (IsWEBPImageLossless((unsigned char *) stream,length) != MagickFalse)
    image->quality=100;
  if (image_info->ping != MagickFalse)
    return(webp_status);
  webp_status=(int) WebPDecode(stream,length,configure);
  if (webp_status != VP8_STATUS_OK)
    return(webp_status);
  p=(unsigned char *) webp_image->u.RGBA.rgba;
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *q;

    ssize_t
      x;

    q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      if (((x >= x_offset) && (x < (x_offset+(ssize_t) image_width))) &&
          ((y >= y_offset) && (y < (y_offset+(ssize_t) image_height))))
        {
          SetPixelRed(image,ScaleCharToQuantum(*p++),q);
          SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
          SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
          SetPixelAlpha(image,ScaleCharToQuantum(*p++),q);
        }
      else
        {
          SetPixelRed(image,(Quantum) 0,q);
          SetPixelGreen(image,(Quantum) 0,q);
          SetPixelBlue(image,(Quantum) 0,q);
          SetPixelAlpha(image,(Quantum) 0,q);
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
  WebPFreeDecBuffer(webp_image);
#if defined(MAGICKCORE_WEBPMUX_DELEGATE)
  {
    StringInfo
      *profile;

    uint32_t
      webp_flags = 0;

    WebPData
     chunk,
     content;

    WebPMux
      *mux;

    /*
      Extract any profiles:
      https://developers.google.com/speed/webp/docs/container-api.
    */
    content.bytes=stream;
    content.size=length;
    mux=WebPMuxCreate(&content,0);
    (void) memset(&chunk,0,sizeof(chunk));
    (void) WebPMuxGetFeatures(mux,&webp_flags);
    if ((webp_flags & ICCP_FLAG) &&
        (WebPMuxGetChunk(mux,"ICCP",&chunk) == WEBP_MUX_OK))
      if (chunk.size != 0)
        {
          profile=BlobToProfileStringInfo("icc",chunk.bytes,chunk.size,
            exception);
          (void) SetImageProfilePrivate(image,profile,exception);
        }
    if ((webp_flags & EXIF_FLAG) &&
        (WebPMuxGetChunk(mux,"EXIF",&chunk) == WEBP_MUX_OK))
      if (chunk.size != 0)
        {
          profile=BlobToProfileStringInfo("exif",chunk.bytes,chunk.size,
            exception);
          (void) SetImageProfilePrivate(image,profile,exception);
        }
    if (((webp_flags & XMP_FLAG) &&
         (WebPMuxGetChunk(mux,"XMP ",&chunk) == WEBP_MUX_OK)) ||
         (WebPMuxGetChunk(mux,"XMP\0",&chunk) == WEBP_MUX_OK))
      if (chunk.size != 0)
        {
          profile=BlobToProfileStringInfo("xmp",chunk.bytes,chunk.size,
            exception);
          (void) SetImageProfilePrivate(image,profile,exception);
        }
    WebPMuxDelete(mux);
  }
#endif
  return(webp_status);
}

#if defined(MAGICKCORE_WEBPMUX_DELEGATE)
static int ReadAnimatedWEBPImage(const ImageInfo *image_info,Image *image,
  uint8_t *stream,size_t length,WebPDecoderConfig *configure,
  ExceptionInfo *exception)
{
  Image
    *original_image;

  int
    image_count,
    webp_status;

  size_t
    canvas_width,
    canvas_height;

  WebPData
    data;

  WebPDemuxer
    *demux;

  WebPIterator
    iter;

  image_count=0;
  webp_status=0;
  original_image=image;
  webp_status=FillBasicWEBPInfo(image,stream,length,configure);
  canvas_width=image->columns;
  canvas_height=image->rows;
  data.bytes=stream;
  data.size=length;
  {
    WebPMux
      *mux;

    WebPMuxAnimParams
      params;

    WebPMuxError
      status;