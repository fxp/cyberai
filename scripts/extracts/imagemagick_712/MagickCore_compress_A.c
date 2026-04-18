// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/compress.c
// Segment 1/11

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%           CCCC   OOO   M   M  PPPP   RRRR   EEEEE   SSSSS  SSSSS            %
%          C      O   O  MM MM  P   P  R   R  E       SS     SS               %
%          C      O   O  M M M  PPPP   RRRR   EEE      SSS    SSS             %
%          C      O   O  M   M  P      R R    E          SS     SS            %
%           CCCC   OOO   M   M  P      R  R   EEEEE   SSSSS  SSSSS            %
%                                                                             %
%                                                                             %
%             MagickCore Image Compression/Decompression Methods              %
%                                                                             %
%                           Software Design                                   %
%                                Cristy                                       %
%                              May  1993                                      %
%                                                                             %
%                                                                             %
%  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization         %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    https://imagemagick.org/license/                                         %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/color-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/compress.h"
#include "MagickCore/constitute.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#if defined(MAGICKCORE_ZLIB_DELEGATE)
#include "zlib.h"
#endif

/*
  Typedef declarations.
*/
struct _Ascii85Info
{
  ssize_t
    offset,
    line_break;

  char
    tuple[6];

  unsigned char
    buffer[10];
};

typedef struct HuffmanTable
{
  size_t
    id,
    code,
    length,
    count;
} HuffmanTable;

/*
  Huffman coding declarations.
*/
#define TWId  23L
#define MWId  24L
#define TBId  25L
#define MBId  26L
#define EXId  27L

static const HuffmanTable
  MBTable[]=
  {
    { MBId, 0x0f, 10, 64 }, { MBId, 0xc8, 12, 128 },
    { MBId, 0xc9, 12, 192 }, { MBId, 0x5b, 12, 256 },
    { MBId, 0x33, 12, 320 }, { MBId, 0x34, 12, 384 },
    { MBId, 0x35, 12, 448 }, { MBId, 0x6c, 13, 512 },
    { MBId, 0x6d, 13, 576 }, { MBId, 0x4a, 13, 640 },
    { MBId, 0x4b, 13, 704 }, { MBId, 0x4c, 13, 768 },
    { MBId, 0x4d, 13, 832 }, { MBId, 0x72, 13, 896 },
    { MBId, 0x73, 13, 960 }, { MBId, 0x74, 13, 1024 },
    { MBId, 0x75, 13, 1088 }, { MBId, 0x76, 13, 1152 },
    { MBId, 0x77, 13, 1216 }, { MBId, 0x52, 13, 1280 },
    { MBId, 0x53, 13, 1344 }, { MBId, 0x54, 13, 1408 },
    { MBId, 0x55, 13, 1472 }, { MBId, 0x5a, 13, 1536 },
    { MBId, 0x5b, 13, 1600 }, { MBId, 0x64, 13, 1664 },
    { MBId, 0x65, 13, 1728 }, { MBId, 0x00, 0, 0 }
  };