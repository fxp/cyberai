// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/gif.c
// Segment 17/18



               datum=GetStringInfoDatum(profile);
               length=GetStringInfoLength(profile);
               (void) WriteBlobByte(image,(unsigned char) 0x21);
               (void) WriteBlobByte(image,(unsigned char) 0xff);
               (void) WriteBlobByte(image,(unsigned char) 0x0b);
               if ((LocaleCompare(name,"ICC") == 0) ||
                   (LocaleCompare(name,"ICM") == 0))
                 {
                   /*
                     Write ICC extension.
                   */
                   (void) WriteBlob(image,11,(unsigned char *) "ICCRGBG1012");
                   (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                     "  Writing GIF Extension %s","ICCRGBG1012");
                 }
               else
                 if ((LocaleCompare(name,"IPTC") == 0))
                   {
                     /*
                       Write IPTC extension.
                     */
                     (void) WriteBlob(image,11,(unsigned char *) "MGKIPTC0000");
                     (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                       "  Writing GIF Extension %s","MGKIPTC0000");
                   }
                 else
                   if ((LocaleCompare(name,"8BIM") == 0))
                     {
                       /*
                         Write 8BIM extension.
                       */
                        (void) WriteBlob(image,11,(unsigned char *)
                          "MGK8BIM0000");
                        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                          "  Writing GIF Extension %s","MGK8BIM0000");
                     }
                   else
                     {
                       char
                         extension[MagickPathExtent];

                       /*
                         Write generic extension.
                       */
                       (void) memset(extension,0,sizeof(extension));
                       (void) CopyMagickString(extension,name+4,
                         sizeof(extension));
                       (void) WriteBlob(image,11,(unsigned char *) extension);
                       (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                         "  Writing GIF Extension %s",name);
                     }
               offset=0;
               while ((ssize_t) length > offset)
               {
                 size_t
                   block_length;