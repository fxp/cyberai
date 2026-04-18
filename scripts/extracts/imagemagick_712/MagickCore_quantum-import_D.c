// Source: /Users/xiaopingfeng/Projects/cyberai/targets/ImageMagick-7.1.2-19/MagickCore/quantum-import.c
// Segment 4/33



  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 8:
    {
      unsigned char
        pixel;

      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushCharPixel(p,&pixel);
        SetPixelBlue(image,ScaleCharToQuantum(pixel),q);
        p=PushCharPixel(p,&pixel);
        SetPixelGreen(image,ScaleCharToQuantum(pixel),q);
        p=PushCharPixel(p,&pixel);
        SetPixelRed(image,ScaleCharToQuantum(pixel),q);
        SetPixelAlpha(image,OpaqueAlpha,q);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 10:
    {
      unsigned int
        pixel;

      range=GetQuantumRange(quantum_info->depth);
      if (quantum_info->pack == MagickFalse)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            SetPixelRed(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,range),q);
            SetPixelGreen(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,range),
              q);
            SetPixelBlue(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,range),q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      if (quantum_info->quantum == 32U)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushQuantumLongPixel(quantum_info,p,&pixel);
            SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
            p=PushQuantumLongPixel(quantum_info,p,&pixel);
            SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
            p=PushQuantumLongPixel(quantum_info,p,&pixel);
            SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
    case 12:
    {
      range=GetQuantumRange(quantum_info->depth);
      if (quantum_info->pack == MagickFalse)
        {
          unsigned short
            pixel;