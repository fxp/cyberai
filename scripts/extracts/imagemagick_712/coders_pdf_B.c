// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 2/30



  StringInfo
    *xmp_profile;
} PDFInfo;

/*
  Forward declarations.
*/
static MagickBooleanType
  WritePDFImage(const ImageInfo *,Image *,ExceptionInfo *),
  WritePOCKETMODImage(const ImageInfo *,Image *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s P D F                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPDF() returns MagickTrue if the image format type, identified by the
%  magick string, is PDF.
%
%  The format of the IsPDF method is:
%
%      MagickBooleanType IsPDF(const unsigned char *magick,const size_t offset)
%
%  A description of each parameter follows:
%
%    o magick: compare image format pattern against these bytes.
%
%    o offset: Specifies the offset of the magick string.
%
*/
static MagickBooleanType IsPDF(const unsigned char *magick,const size_t offset)
{
  if (offset < 5)
    return(MagickFalse);
  if (LocaleNCompare((const char *) magick,"%PDF-",5) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P D F I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPDFImage() reads a Portable Document Format image file and
%  returns it.  It allocates the memory necessary for the new Image structure
%  and returns a pointer to the new image.
%
%  The format of the ReadPDFImage method is:
%
%      Image *ReadPDFImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static void ReadPDFInfo(const ImageInfo *image_info,Image *image,
  PDFInfo *pdf_info,ExceptionInfo *exception)
{
#define CMYKProcessColor  "CMYKProcessColor"
#define CropBox  "CropBox"
#define DefaultCMYK  "DefaultCMYK"
#define DeviceCMYK  "DeviceCMYK"
#define MediaBox  "MediaBox"
#define PDFRotate  "Rotate"
#define SpotColor  "Separation"
#define TrimBox  "TrimBox"
#define PDFVersion  "PDF-"

  char
    version[MagickPathExtent];

  int
    c,
    percent_count;

  MagickByteBuffer
    buffer;

  char
    *p;

  ssize_t
    i;

  SegmentInfo
    bounds;

  size_t
    spotcolor;

  ssize_t
    count;