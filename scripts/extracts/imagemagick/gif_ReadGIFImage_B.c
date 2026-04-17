/* ===== EXTRACT: gif.c ===== */
/* Function: ReadGIFImage (part B) */
/* Lines: 1111–1216 */

              if ((count+(ssize_t) offset+MagickPathExtent) >= (ssize_t) extent)
                {
                  extent<<=1;
                  comments=(char *) ResizeQuantumMemory(comments,
                    OverAllocateMemory(extent+MagickPathExtent),
                    sizeof(*comments));
                  if (comments == (char *) NULL)
                    ThrowGIFException(ResourceLimitError,
                      "MemoryAllocationFailed");
                }
              (void) CopyMagickString(&comments[offset],(char *) buffer,extent-
                offset);
            }
            (void) SetImageProperty(meta_image,"comment",comments,exception);
            comments=DestroyString(comments);
            break;
          }
          case 0xff:
          {
            MagickBooleanType
              loop;

            /*
              Read Netscape Loop extension.
            */
            loop=MagickFalse;
            if (ReadBlobBlock(image,buffer) != 0)
              loop=LocaleNCompare((char *) buffer,"NETSCAPE2.0",11) == 0 ?
                MagickTrue : MagickFalse;
            if (loop != MagickFalse)
              while (ReadBlobBlock(image,buffer) != 0)
              {
                meta_image->iterations=((size_t) buffer[2] << 8) | buffer[1];
                if (meta_image->iterations != 0)
                  meta_image->iterations++;
              }
            else
              {
                char
                  name[MagickPathExtent];

                int
                  block_length,
                  info_length,
                  reserved_length;

                MagickBooleanType
                  magick = MagickFalse;

                unsigned char
                  *info;

                /*
                  Store GIF application extension as a generic profile.
                */
                if (LocaleNCompare((char *) buffer,"ImageMagick",11) == 0)
                  magick=MagickTrue;
                else if (LocaleNCompare((char *) buffer,"ICCRGBG1012",11) == 0)
                  (void) CopyMagickString(name,"icc",sizeof(name));
                else if (LocaleNCompare((char *) buffer,"MGK8BIM0000",11) == 0)
                  (void) CopyMagickString(name,"8bim",sizeof(name));
                else if (LocaleNCompare((char *) buffer,"MGKIPTC0000",11) == 0)
                  (void) CopyMagickString(name,"iptc",sizeof(name));
                else
                  (void) FormatLocaleString(name,sizeof(name),"gif:%.11s",
                    buffer);
                reserved_length=255;
                info=(unsigned char *) AcquireQuantumMemory((size_t)
                  reserved_length,sizeof(*info));
                if (info == (unsigned char *) NULL)
                  ThrowGIFException(ResourceLimitError,
                    "MemoryAllocationFailed");
                (void) memset(info,0,(size_t) reserved_length*sizeof(*info));
                for (info_length=0; ; )
                {
                  block_length=(int) ReadBlobBlock(image,info+info_length);
                  if (block_length == 0)
                    break;
                  info_length+=block_length;
                  if (info_length > (reserved_length-255))
                    {
                      reserved_length+=4096;
                      info=(unsigned char *) ResizeQuantumMemory(info,(size_t)
                        reserved_length,sizeof(*info));
                      if (info == (unsigned char *) NULL)
                        {
                          info=(unsigned char *) RelinquishMagickMemory(info);
                          ThrowGIFException(ResourceLimitError,
                            "MemoryAllocationFailed");
                        }
                    }
                }
                if (magick != MagickFalse)
                  meta_image->gamma=StringToDouble((char *) info+6,
                      (char **) NULL);
                else
                  {
                    StringInfo
                      *profile;

                    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                      "      profile name=%s",name);
                    profile=BlobToProfileStringInfo(name,info,(size_t) info_length,
                      exception);
                    if (profile != (StringInfo *) NULL)
                      {
