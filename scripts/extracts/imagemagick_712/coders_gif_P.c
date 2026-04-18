// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 16/18



        if ((GetPreviousImageInList(image) == (Image *) NULL) &&
            (GetNextImageInList(image) != (Image *) NULL) &&
            (image->iterations != 1))
          {
            /*
              Write Netscape Loop extension.
            */
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
               "  Writing GIF Extension %s","NETSCAPE2.0");
            (void) WriteBlobByte(image,(unsigned char) 0x21);
            (void) WriteBlobByte(image,(unsigned char) 0xff);
            (void) WriteBlobByte(image,(unsigned char) 0x0b);
            (void) WriteBlob(image,11,(unsigned char *) "NETSCAPE2.0");
            (void) WriteBlobByte(image,(unsigned char) 0x03);
            (void) WriteBlobByte(image,(unsigned char) 0x01);
            (void) WriteBlobLSBShort(image,(unsigned short) (image->iterations ?
              image->iterations-1 : 0));
            (void) WriteBlobByte(image,(unsigned char) 0x00);
          }
        /*
          Write graphics control extension.
        */
        (void) WriteBlobByte(image,(unsigned char) 0x21);
        (void) WriteBlobByte(image,(unsigned char) 0xf9);
        (void) WriteBlobByte(image,(unsigned char) 0x04);
        c=(int) (image->dispose << 2);
        if (opacity >= 0)
          c|=0x01;
        (void) WriteBlobByte(image,(unsigned char) c);
        delay=(size_t) (100*image->delay/MagickMax((size_t)
          image->ticks_per_second,1));
        (void) WriteBlobLSBShort(image,(unsigned short) delay);
        (void) WriteBlobByte(image,(unsigned char) (opacity >= 0 ? opacity :
          0));
        (void) WriteBlobByte(image,(unsigned char) 0x00);
        if (fabs(image->gamma-1.0/2.2) > MagickEpsilon)
          {
            char
              attributes[MagickPathExtent];

            ssize_t
              count;

            /*
              Write ImageMagick extension.
            */
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
               "  Writing GIF Extension %s","ImageMagick");
            (void) WriteBlobByte(image,(unsigned char) 0x21);
            (void) WriteBlobByte(image,(unsigned char) 0xff);
            (void) WriteBlobByte(image,(unsigned char) 0x0b);
            (void) WriteBlob(image,11,(unsigned char *) "ImageMagick");
            count=FormatLocaleString(attributes,MagickPathExtent,"gamma=%g",
              image->gamma);
            (void) WriteBlobByte(image,(unsigned char) count);
            (void) WriteBlob(image,(size_t) count,(unsigned char *) attributes);
            (void) WriteBlobByte(image,(unsigned char) 0x00);
          }
        value=GetImageProperty(image,"comment",exception);
        if (value != (const char *) NULL)
          {
            const char
              *p;

            size_t
              count;

            /*
              Write comment extension.
            */
            (void) WriteBlobByte(image,(unsigned char) 0x21);
            (void) WriteBlobByte(image,(unsigned char) 0xfe);
            for (p=value; *p != '\0'; )
            {
              count=MagickMin(strlen(p),255);
              (void) WriteBlobByte(image,(unsigned char) count);
              for (i=0; i < (ssize_t) count; i++)
                (void) WriteBlobByte(image,(unsigned char) *p++);
            }
            (void) WriteBlobByte(image,(unsigned char) 0x00);
          }
        ResetImageProfileIterator(image);
        for ( ; ; )
        {
          char
            *name;

          const StringInfo
            *profile;

          name=GetNextImageProfile(image);
          if (name == (const char *) NULL)
            break;
          profile=GetImageProfile(image,name);
          if (profile != (StringInfo *) NULL)
          {
            if ((LocaleCompare(name,"ICC") == 0) ||
                (LocaleCompare(name,"ICM") == 0) ||
                (LocaleCompare(name,"IPTC") == 0) ||
                (LocaleCompare(name,"8BIM") == 0) ||
                (LocaleNCompare(name,"gif:",4) == 0))
            {
               ssize_t
                 offset;

               unsigned char
                 *datum;