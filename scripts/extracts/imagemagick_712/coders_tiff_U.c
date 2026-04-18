// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/tiff.c
// Segment 21/34

                                                                   %
%                                                                             %
%                                                                             %
%   W r i t e G R O U P 4 I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteGROUP4Image() writes an image in the raw CCITT Group 4 image format.
%
%  The format of the WriteGROUP4Image method is:
%
%      MagickBooleanType WriteGROUP4Image(const ImageInfo *image_info,
%        Image *image,ExceptionInfo *)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image:  The image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType WriteGROUP4Image(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent];

  FILE
    *file;

  Image
    *huffman_image;

  ImageInfo
    *write_info;

  int
    unique_file;

  MagickBooleanType
    status;

  ssize_t
    i;

  ssize_t
    count;

  TIFF
    *tiff;

  toff_t
    *byte_count,
    strip_size;

  unsigned char
    *buffer;