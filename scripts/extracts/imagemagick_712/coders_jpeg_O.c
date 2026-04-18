// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/jpeg.c
// Segment 15/24

oid) UnregisterMagickInfo("JPS");
  (void) UnregisterMagickInfo("JPG");
  (void) UnregisterMagickInfo("JPEG");
  (void) UnregisterMagickInfo("JPE");
}

#if defined(MAGICKCORE_JPEG_DELEGATE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  W r i t e J P E G I m a g e                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteJPEGImage() writes a JPEG image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the WriteJPEGImage method is:
%
%      MagickBooleanType WriteJPEGImage(const ImageInfo *image_info,
%        Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o jpeg_image:  The image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static QuantizationTable *DestroyQuantizationTable(QuantizationTable *table)
{
  assert(table != (QuantizationTable *) NULL);
  if (table->slot != (char *) NULL)
    table->slot=DestroyString(table->slot);
  if (table->description != (char *) NULL)
    table->description=DestroyString(table->description);
  if (table->levels != (unsigned int *) NULL)
    table->levels=(unsigned int *) RelinquishMagickMemory(table->levels);
  table=(QuantizationTable *) RelinquishMagickMemory(table);
  return(table);
}

static boolean EmptyOutputBuffer(j_compress_ptr compress_info)
{
  DestinationManager
    *destination;

  destination=(DestinationManager *) compress_info->dest;
  destination->manager.free_in_buffer=(size_t) WriteBlob(destination->image,
    MagickMinBufferExtent,destination->buffer);
  if (destination->manager.free_in_buffer != MagickMinBufferExtent)
    ERREXIT(compress_info,JERR_FILE_WRITE);
  destination->manager.next_output_byte=destination->buffer;
  return(TRUE);
}

static QuantizationTable *GetQuantizationTable(const char *filename,
  const char *slot,ExceptionInfo *exception)
{
  char
    *p,
    *xml;

  const char
    *attribute,
    *content;

  double
    value;

  QuantizationTable
    *table;

  size_t
    length;

  ssize_t
    i,
    j;

  XMLTreeInfo
    *description,
    *levels,
    *quantization_tables,
    *table_iterator;