/* ===== EXTRACT: bmp.c ===== */
/* Function: ReadBMPImage (part C) */
/* Lines: 858–966 */

        if (bmp_info.size > 40)
          {
            double
              gamma;

            /*
              Read color management information.
            */
            bmp_info.alpha_mask=ReadBlobLSBLong(image);
            bmp_info.colorspace=ReadBlobLSBSignedLong(image);
            /*
              Decode 2^30 fixed point formatted CIE primaries.
            */
#           define BMP_DENOM ((double) 0x40000000)
            bmp_info.red_primary.x=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.red_primary.y=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.red_primary.z=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.green_primary.x=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.green_primary.y=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.green_primary.z=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.blue_primary.x=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.blue_primary.y=(double) ReadBlobLSBLong(image)/BMP_DENOM;
            bmp_info.blue_primary.z=(double) ReadBlobLSBLong(image)/BMP_DENOM;

            gamma=bmp_info.red_primary.x+bmp_info.red_primary.y+
              bmp_info.red_primary.z;
            gamma=PerceptibleReciprocal(gamma);
            bmp_info.red_primary.x*=gamma;
            bmp_info.red_primary.y*=gamma;

            gamma=bmp_info.green_primary.x+bmp_info.green_primary.y+
              bmp_info.green_primary.z;
            gamma=PerceptibleReciprocal(gamma);
            bmp_info.green_primary.x*=gamma;
            bmp_info.green_primary.y*=gamma;

            gamma=bmp_info.blue_primary.x+bmp_info.blue_primary.y+
              bmp_info.blue_primary.z;
            gamma=PerceptibleReciprocal(gamma);
            bmp_info.blue_primary.x*=gamma;
            bmp_info.blue_primary.y*=gamma;

            /*
              Decode 16^16 fixed point formatted gamma_scales.
            */
            bmp_info.gamma_scale.x=(double) ReadBlobLSBLong(image)/0x10000;
            bmp_info.gamma_scale.y=(double) ReadBlobLSBLong(image)/0x10000;
            bmp_info.gamma_scale.z=(double) ReadBlobLSBLong(image)/0x10000;

            if (bmp_info.colorspace == 0)
              {
                image->chromaticity.red_primary.x=bmp_info.red_primary.x;
                image->chromaticity.red_primary.y=bmp_info.red_primary.y;
                image->chromaticity.green_primary.x=bmp_info.green_primary.x;
                image->chromaticity.green_primary.y=bmp_info.green_primary.y;
                image->chromaticity.blue_primary.x=bmp_info.blue_primary.x;
                image->chromaticity.blue_primary.y=bmp_info.blue_primary.y;
                /*
                  Compute a single gamma from the BMP 3-channel gamma.
                */
                image->gamma=(bmp_info.gamma_scale.x+bmp_info.gamma_scale.y+
                  bmp_info.gamma_scale.z)/3.0;
              }
          }
        else
          (void) CopyMagickString(image->magick,"BMP3",MagickPathExtent);

        if (bmp_info.size > 108)
          {
            size_t
              intent;

            /*
              Read BMP Version 5 color management information.
            */
            intent=ReadBlobLSBLong(image);
            switch ((int) intent)
            {
              case LCS_GM_BUSINESS:
              {
                image->rendering_intent=SaturationIntent;
                break;
              }
              case LCS_GM_GRAPHICS:
              {
                image->rendering_intent=RelativeIntent;
                break;
              }
              case LCS_GM_IMAGES:
              {
                image->rendering_intent=PerceptualIntent;
                break;
              }
              case LCS_GM_ABS_COLORIMETRIC:
              {
                image->rendering_intent=AbsoluteIntent;
                break;
              }
            }
            profile_data=(MagickOffsetType) ReadBlobLSBLong(image);
            profile_size=(MagickOffsetType) ReadBlobLSBLong(image);
            (void) ReadBlobLSBLong(image);  /* Reserved byte */
          }
      }
    if ((ignore_filesize == MagickFalse) &&
        (MagickSizeType) bmp_info.file_size != blob_size)
      (void) ThrowMagickException(exception,GetMagickModule(),
        CorruptImageError,"LengthAndFilesizeDoNotMatch","`%s'",
        image->filename);
