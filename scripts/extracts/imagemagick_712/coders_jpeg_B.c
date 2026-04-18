// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 2/24

axJPEGScans  1024

/*
  Typedef declarations.
*/
#if defined(MAGICKCORE_JPEG_DELEGATE)
typedef struct _DestinationManager
{
  struct jpeg_destination_mgr
    manager;

  Image
    *image;

  JOCTET
    *buffer;
} DestinationManager;

typedef struct _JPEGClientInfo
{
  jmp_buf
    error_recovery;

  Image
    *image;

  MagickBooleanType
    finished;

  StringInfo
    *profiles[MaxJPEGProfiles+1];

  ExceptionInfo
    *exception;
} JPEGClientInfo;

typedef struct _SourceManager
{
  struct jpeg_source_mgr
    manager;

  Image
    *image;

  JOCTET
    *buffer;

  boolean
    start_of_blob;
} SourceManager;
#endif

typedef struct _QuantizationTable
{
  char
    *slot,
    *description;

  size_t
    width,
    height;

  double
    divisor;

  unsigned int
    *levels;
} QuantizationTable;

/*
  Const declarations.
*/
static const char
  xmp_namespace[] = "http://ns.adobe.com/xap/1.0/";

/*
  Forward declarations.
*/
#if defined(MAGICKCORE_JPEG_DELEGATE)
static MagickBooleanType
  WriteJPEGImage(const ImageInfo *,Image *,ExceptionInfo *);
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s J P E G                                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsJPEG() returns MagickTrue if the image format type, identified by the
%  magick string, is JPEG.
%
%  The format of the IsJPEG  method is:
%
%      MagickBooleanType IsJPEG(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: compare image format pattern against these bytes.
%
%    o length: Specifies the length of the magick string.
%
*/
static MagickBooleanType IsJPEG(const unsigned char *magick,const size_t length)
{
  if (length < 3)
    return(MagickFalse);
  if (memcmp(magick,"\377\330\377",3) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

#if defined(MAGICKCORE_JPEG_DELEGATE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d J P E G I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadJPEGImage() reads a JPEG image file and returns it.  It allocates
%  the memory necessary for the new Image structure and returns a pointer to
%  the new image.
%
%  The format of the ReadJPEGImage method is:
%
%      Image *ReadJPEGImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline Quantum JPEGGetQuantum(const struct jpeg_decompress_struct *jpeg_info,
  QuantumAny range,const JSAMPLE *p)
{
  if (jpeg_info->data_precision == 8)
    return ScaleCharToQuantum(*p);
  else if (jpeg_info->data_precision == 16)
    return ScaleShortToQuantum(*(unsigned short *) p);
  else if (jpeg_info->data_precision < 8)
    return ScaleAnyToQuantum(*p,range);
  else
    return ScaleAnyToQuantum(*(unsigned short *) p,range);
}

static boolean FillInputBuffer(const j_decompress_ptr compress_info)
{
  SourceManager
    *source;

  source=(SourceManager *) compress_info->src;
  source->manager.bytes_in_buffer=(size_t) ReadBlob(source->image,
    MagickMinBufferExtent,source->buffer);
  if (source->manager.bytes_in_buffer == 0)
    {
      if (source->start_of_blob != FALSE)
        ERREXIT(compress_info,JERR_INPUT_EMPTY);
      WARNMS(compress_info,JWRN_JPEG_EOF);
      source->buffer[0]=(JOCTET) 0xff;
      source->buffer[1]=(JOCTET) JPEG_EOI;
      source->manager.bytes_in_buffer=2;
    }
  source->manager.next_input_byte=source->buffer;
  source->start_of_blob=FALSE;
  return(TRUE);
}