// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/png.c
// Segment 57/94



   if (logging != MagickFalse)
     {
       (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "      image->columns=%.20g",(double) image->columns);
       (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "      image->rows=%.20g",(double) image->rows);
       (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "      image->alpha_trait=%.20g",(double) image->alpha_trait);
       (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "      image->depth=%.20g",(double) image->depth);

       if (image->storage_class == PseudoClass && image->colormap != NULL)
       {
         (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "      Original colormap:");
         (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "        i    (red,green,blue,alpha)");

         for (i=0; i < (ssize_t) MagickMin(image->colors,256); i++)
         {
               (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "        %d    (%d,%d,%d,%d)",
                    (int) i,
                    (int) image->colormap[i].red,
                    (int) image->colormap[i].green,
                    (int) image->colormap[i].blue,
                    (int) image->colormap[i].alpha);
         }

         for (i=(ssize_t) image->colors - 10; i < (ssize_t) image->colors; i++)
         {
           if (i > 255)
             {
               (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "        %d    (%d,%d,%d,%d)",
                    (int) i,
                    (int) image->colormap[i].red,
                    (int) image->colormap[i].green,
                    (int) image->colormap[i].blue,
                    (int) image->colormap[i].alpha);
             }
         }
       }

       (void) LogMagickEvent(CoderEvent,GetMagickModule(),
           "      image->colors=%d",(int) image->colors);

       if (image->colors == 0)
         (void) LogMagickEvent(CoderEvent,GetMagickModule(),
             "        (zero means unknown)");

       if (mng_info->preserve_colormap == MagickFalse)
         (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "      Regenerate the colormap");
     }

     image_colors=0;
     number_opaque = 0;
     number_semitransparent = 0;
     number_transparent = 0;

     for (y=0; y < (ssize_t) image->rows; y++)
     {
       r=GetVirtualPixels(image,0,y,image->columns,1,exception);

       if (r == (const Quantum *) NULL)
         break;

       for (x=0; x < (ssize_t) image->columns; x++)
       {
           if ((((image->alpha_trait & BlendPixelTrait) == 0) &&
                ((image->alpha_trait & UpdatePixelTrait) == 0)) ||
               (GetPixelAlpha(image,r) == OpaqueAlpha))
             {
               if (number_opaque < 259)
                 {
                   if (number_opaque == 0)
                     {
                       GetPixelInfoPixel(image,r,opaque);
                       opaque[0].alpha=OpaqueAlpha;
                       number_opaque=1;
                     }

                   for (i=0; i< (ssize_t) number_opaque; i++)
                     {
                       if (IsColorEqual(image,r,opaque+i))
                         break;
                     }

                   if (i ==  (ssize_t) number_opaque && number_opaque < 259)
                     {
                       number_opaque++;
                       GetPixelInfoPixel(image,r,opaque+i);
                       opaque[i].alpha=OpaqueAlpha;
                     }
                 }
             }
           else if (GetPixelAlpha(image,r) == TransparentAlpha)
             {
               if (number_transparent < 259)
                 {
                   if (number_transparent == 0)
                     {
                       GetPixelInfoPixel(image,r,transparent);
                       ping_trans_color.red=(unsigned short)
                         GetPixelRed(image,r);
                       ping_trans_color.green=(unsigned short)
                         GetPixelGreen(image,r);
                       ping_trans_color.blue=(unsigned short)
                         GetPixelBlue(image,r);
                       ping_trans_color.gray=(unsigned short)
                         GetPixelGray(image,r);
                       number_transparent = 1;
                     }

                   for (i=0; i< (ssize_t) number_transparent; i++)
                     {
                       if (IsColorEqual(image,r,transparent+i))
                         break;
                     }