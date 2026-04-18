// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/coders/pdf.c
// Segment 12/30



  if (*value == '\0')
    return;
  if (is_pdfa != MagickFalse)
    {
      escaped=EscapeParenthesis(value);
      (void) WriteBlobString(image,"/");
      (void) WriteBlobString(image,keyword);
      (void) WriteBlobString(image," (");
      (void) WriteBlobString(image,escaped);
      escaped=DestroyString(escaped);
      (void) WriteBlobString(image,")\n");
      return;
    }
  utf16=ConvertUTF8ToUTF16((const unsigned char *) value,&length);
  if (utf16 != (wchar_t *) NULL)
    {
      unsigned char
        hex_digits[16];

      hex_digits[0]='0';
      hex_digits[1]='1';
      hex_digits[2]='2';
      hex_digits[3]='3';
      hex_digits[4]='4';
      hex_digits[5]='5';
      hex_digits[6]='6';
      hex_digits[7]='7';
      hex_digits[8]='8';
      hex_digits[9]='9';
      hex_digits[10]='A';
      hex_digits[11]='B';
      hex_digits[12]='C';
      hex_digits[13]='D';
      hex_digits[14]='E';
      hex_digits[15]='F';
      (void) WriteBlobString(image,"/");
      (void) WriteBlobString(image,keyword);
      (void) WriteBlobString(image," <FEFF");
      for (i=0; i < (ssize_t) length - 1; i++)
      {
        (void) WriteBlobByte(image,hex_digits[(utf16[i] >> 12) & 0x0f]);
        (void) WriteBlobByte(image,hex_digits[(utf16[i] >> 8) & 0x0f]);
        (void) WriteBlobByte(image,hex_digits[(utf16[i] >> 4) & 0x0f]);
        (void) WriteBlobByte(image,hex_digits[utf16[i] & 0x0f]);
      }
      (void) WriteBlobString(image,">\n");
      utf16=(wchar_t *) RelinquishMagickMemory(utf16);
    }
}

static const StringInfo *GetCompatibleColorProfile(const Image* image)
{
  ColorspaceType
    colorspace;

  const StringInfo
    *icc_profile;

  colorspace=UndefinedColorspace;
  icc_profile=GetImageProfile(image,"icc");
  if (icc_profile == (const StringInfo *) NULL)
    return((const StringInfo *) NULL);
  if (GetStringInfoLength(icc_profile) > 20)
    {
      const char
        *p;

      unsigned int
        value;

      p=(const char *) GetStringInfoDatum(icc_profile)+16;
      value=(unsigned int) (*p++) << 24;
      value|=(unsigned int) (*p++) << 16;
      value|=(unsigned int) (*p++) << 8;
      value|=(unsigned int) *p;
      switch (value)
      {
        case 0x58595a20:
          colorspace=XYZColorspace;
          break;
        case 0x4c616220:
          colorspace=LabColorspace;
          break;
        case 0x4c757620:
          colorspace=LuvColorspace;
          break;
        case 0x59436272:
          colorspace=YCbCrColorspace;
          break;
        case 0x52474220:
          if ((image->colorspace == sRGBColorspace) ||
              (image->colorspace == RGBColorspace))
            return(icc_profile);
          break;
        case 0x47524159:
          colorspace=GRAYColorspace;
          break;
        case 0x48535620:
          colorspace=HSVColorspace;
          break;
        case 0x434D594B:
          colorspace=CMYKColorspace;
          break;
        case 0x434D5920:
          colorspace=CMYColorspace;
          break;
      }
    }
  if (image->colorspace == colorspace)
    return(icc_profile);
  return((const StringInfo *) NULL);
}

static MagickBooleanType WritePDFImage(const ImageInfo *image_info,Image *image,
  ExceptionInfo *exception)
{
#define CFormat  "/Filter [ /%s ]\n"
#define ObjectsPerImage  14
#define ThrowPDFException(exception,message) \
{ \
  if (xref != (MagickOffsetType *) NULL) \
    xref=(MagickOffsetType *) RelinquishMagickMemory(xref); \
  ThrowWriterException((exception),(message)); \
}