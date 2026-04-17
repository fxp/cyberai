/* ===== EXTRACT: gif.c ===== */
/* Function: ReadGIFImage (part A) */
/* Lines: 965–1110 */

static Image *ReadGIFImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define BitSet(byte,bit)  (((byte) & (bit)) == (bit))
#define LSBFirstOrder(x,y)  (((y) << 8) | (x))
#define ThrowGIFException(exception,message) \
{ \
  if (profiles != (LinkedListInfo *) NULL) \
    profiles=DestroyLinkedList(profiles,DestroyGIFProfile); \
  if (global_colormap != (unsigned char *) NULL) \
    global_colormap=(unsigned char *) RelinquishMagickMemory(global_colormap); \
  if (meta_image != (Image *) NULL) \
    meta_image=DestroyImage(meta_image); \
  ThrowReaderException((exception),(message)); \
}

  Image
    *image,
    *meta_image;

  LinkedListInfo
    *profiles;

  MagickBooleanType
    status;

  ssize_t
    i;

  unsigned char
    *p;

  size_t
    duration,
    global_colors,
    image_count,
    local_colors,
    one;

  ssize_t
    count,
    opacity;

  unsigned char
    background,
    buffer[257],
    c,
    flag,
    *global_colormap;

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
  /*
    Determine if this a GIF file.
  */
  count=ReadBlob(image,6,buffer);
  if ((count != 6) || ((LocaleNCompare((char *) buffer,"GIF87",5) != 0) &&
      (LocaleNCompare((char *) buffer,"GIF89",5) != 0)))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  (void) memset(buffer,0,sizeof(buffer));
  meta_image=AcquireImage(image_info,exception);  /* metadata container */
  meta_image->page.width=ReadBlobLSBShort(image);
  meta_image->page.height=ReadBlobLSBShort(image);
  meta_image->iterations=1;
  flag=(unsigned char) ReadBlobByte(image);
  profiles=(LinkedListInfo *) NULL;
  background=(unsigned char) ReadBlobByte(image);
  c=(unsigned char) ReadBlobByte(image);  /* reserved */
  one=1;
  global_colors=one << (((size_t) flag & 0x07)+1);
  global_colormap=(unsigned char *) AcquireQuantumMemory((size_t)
    MagickMax(global_colors,256),3UL*sizeof(*global_colormap));
  if (global_colormap == (unsigned char *) NULL)
    ThrowGIFException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memset(global_colormap,0,3*MagickMax(global_colors,256)*
    sizeof(*global_colormap));
  if (BitSet((int) flag,0x80) != 0)
    {
      count=ReadBlob(image,(size_t) (3*global_colors),global_colormap);
      if (count != (ssize_t) (3*global_colors))
        ThrowGIFException(CorruptImageError,"InsufficientImageDataInFile");
    }
  duration=0;
  opacity=(-1);
  image_count=0;
  for ( ; ; )
  {
    count=ReadBlob(image,1,&c);
    if (count != 1)
      break;
    if (c == (unsigned char) ';')
      break;  /* terminator */
    if (c == (unsigned char) '!')
      {
        /*
          GIF Extension block.
        */
        (void) memset(buffer,0,sizeof(buffer));
        count=ReadBlob(image,1,&c);
        if (count != 1)
          ThrowGIFException(CorruptImageError,"UnableToReadExtensionBlock");
        switch (c)
        {
          case 0xf9:
          {
            /*
              Read graphics control extension.
            */
            while (ReadBlobBlock(image,buffer) != 0) ;
            meta_image->dispose=(DisposeType) ((buffer[0] >> 2) & 0x07);
            meta_image->delay=((size_t) buffer[2] << 8) | buffer[1];
            if ((ssize_t) (buffer[0] & 0x01) == 0x01)
              opacity=(ssize_t) buffer[3];
            break;
          }
          case 0xfe:
          {
            char
              *comments;

            size_t
              extent,
              offset;

            comments=AcquireString((char *) NULL);
            extent=MagickPathExtent;
            for (offset=0; ; offset+=(size_t) count)
            {
              count=ReadBlobBlock(image,buffer);
              if (count == 0)
                break;
              buffer[count]='\0';
