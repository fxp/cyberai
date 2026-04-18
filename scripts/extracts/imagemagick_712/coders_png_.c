// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 77/94



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P N G I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WritePNGImage() writes a Portable Network Graphics (PNG) or
%  Multiple-image Network Graphics (MNG) image file.
%
%  MNG support written by Glenn Randers-Pehrson, glennrp@image...
%
%  The format of the WritePNGImage method is:
%
%      MagickBooleanType WritePNGImage(const ImageInfo *image_info,
%        Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image:  The image.
%
%    o exception: return any errors or warnings in this structure.
%
%  Returns MagickTrue on success, MagickFalse on failure.
%
%  Communicating with the PNG encoder:
%
%  While the datastream written is always in PNG format and normally would
%  be given the "png" file extension, this method also writes the following
%  pseudo-formats which are subsets of png:
%
%    o PNG8:    An 8-bit indexed PNG datastream is written.  If the image has
%               a depth greater than 8, the depth is reduced. If transparency
%               is present, the tRNS chunk must only have values 0 and 255
%               (i.e., transparency is binary: fully opaque or fully
%               transparent).  If other values are present they will be
%               50%-thresholded to binary transparency.  If more than 256
%               colors are present, they will be quantized to the 4-4-4-1,
%               3-3-3-1, or 3-3-2-1 palette.  The underlying RGB color
%               of any resulting fully-transparent pixels is changed to
%               the image's background color.
%
%               If you want better quantization or dithering of the colors
%               or alpha than that, you need to do it before calling the
%               PNG encoder. The pixels contain 8-bit indices even if
%               they could be represented with 1, 2, or 4 bits.  Grayscale
%               images will be written as indexed PNG files even though the
%               PNG grayscale type might be slightly more efficient.  Please
%               note that writing to the PNG8 format may result in loss
%               of color and alpha data.
%
%    o PNG24:   An 8-bit per sample RGB PNG datastream is written.  The tRNS
%               chunk can be present to convey binary transparency by naming
%               one of the colors as transparent.  The only loss incurred
%               is reduction of sample depth to 8.  If the image has more
%               than one transparent color, has semitransparent pixels, or
%               has an opaque pixel with the same RGB components as the
%               transparent color, an image is not written.
%
%    o PNG32:   An 8-bit per sample RGBA PNG is written.  Partial
%               transparency is permitted, i.e., the alpha sample for
%               each pixel can have any value from 0 to 255. The alpha
%               channel is present even if the image is fully opaque.
%               The only loss in data is the reduction of the sample depth
%               to 8.
%
%    o PNG48:   A 16-bit per sample RGB PNG datastream is written.  The tRNS
%               chunk can be present to convey binary transparency by naming
%               one of the colors as transparent.  If the image has more
%               than one transparent color, has semitransparent pixels, or
%               has an opaque pixel with the same RGB components as the
%               transparent color, an image is not written.
%
%    o PNG64:   A 16-bit per sample RGBA PNG is written.  Partial
%               transparency is permitted, i.e., the alpha sample for
%               each pixel can have any value from 0 to 65535. The alpha
%               channel is present even if the image is fully opaque.
%
%    o PNG00:   A PNG that inherits its colortype and bit-depth from the input
%               image, if the input was a PNG, is written.  If these values
%               cannot be found, or if the pixels have been changed in a way
%               that makes this impossible, then "PNG00" falls back to the
%               regular "PNG" format.
%
%    o -define: For more precise control of the PNG output, you can use the
%               Image options "png:bit-depth" and "png:color-type".  These
%               can be set from the commandline with "-define" and a