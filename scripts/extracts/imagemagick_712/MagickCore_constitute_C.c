// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/constitute.c
// Segment 3/15



  /*
    Allocate image structure.
  */
  assert(map != (const char *) NULL);
  assert(pixels != (void *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",map);
  image=AcquireImage((ImageInfo *) NULL,exception);
  if (image == (Image *) NULL)
    return((Image *) NULL);
  switch (storage)
  {
    case CharPixel: image->depth=8*sizeof(unsigned char); break;
    case DoublePixel: image->depth=8*sizeof(double); break;
    case FloatPixel: image->depth=8*sizeof(float); break;
    case LongPixel: image->depth=8*sizeof(unsigned long); break;
    case LongLongPixel: image->depth=8*sizeof(MagickSizeType); break;
    case ShortPixel: image->depth=8*sizeof(unsigned short); break;
    default: break;
  }
  length=strlen(map);
  for (i=0; i < (ssize_t) length; i++)
  {
    switch (map[i])
    {
      case 'a':
      case 'A':
      case 'O':
      case 'o':
      {
        image->alpha_trait=BlendPixelTrait;
        break;
      }
      case 'C':
      case 'c':
      case 'm':
      case 'M':
      case 'Y':
      case 'y':
      case 'K':
      case 'k':
      {
        image->colorspace=CMYKColorspace;
        break;
      }
      case 'I':
      case 'i':
      {
        image->colorspace=GRAYColorspace;
        break;
      }
      default:
      {
        if (length == 1)
          image->colorspace=GRAYColorspace;
        break;
      }
    }
  }
  status=SetImageExtent(image,columns,rows,exception);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  status=ResetImagePixels(image,exception);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  status=ImportImagePixels(image,0,0,columns,rows,map,storage,pixels,exception);
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i n g I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PingImage() returns all the properties of an image or image sequence
%  except for the pixels.  It is much faster and consumes far less memory
%  than ReadImage().  On failure, a NULL image is returned and exception
%  describes the reason for the failure.
%
%  The format of the PingImage method is:
%
%      Image *PingImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: Ping the image defined by the file or filename members of
%      this structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static size_t PingStream(const Image *magick_unused(image),
  const void *magick_unused(pixels),const size_t columns)
{
  magick_unreferenced(image);
  magick_unreferenced(pixels);
  return(columns);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport Image *PingImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  Image
    *image;

  ImageInfo
    *ping_info;