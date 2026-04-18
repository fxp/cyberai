// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 35/38



            // set up the typical prediction context
            tpgrCX0 = tpgrCX1 = tpgrCX2 = 0; // make gcc happy
            if (tpgrOn) {
                refBitmap->getPixelPtr(-1 - refDX, y - 1 - refDY, &tpgrCXPtr0);
                tpgrCX0 = refBitmap->nextPixel(&tpgrCXPtr0);
                tpgrCX0 = (tpgrCX0 << 1) | refBitmap->nextPixel(&tpgrCXPtr0);
                tpgrCX0 = (tpgrCX0 << 1) | refBitmap->nextPixel(&tpgrCXPtr0);
                refBitmap->getPixelPtr(-1 - refDX, y - refDY, &tpgrCXPtr1);
                tpgrCX1 = refBitmap->nextPixel(&tpgrCXPtr1);
                tpgrCX1 = (tpgrCX1 << 1) | refBitmap->nextPixel(&tpgrCXPtr1);
                tpgrCX1 = (tpgrCX1 << 1) | refBitmap->nextPixel(&tpgrCXPtr1);
                refBitmap->getPixelPtr(-1 - refDX, y + 1 - refDY, &tpgrCXPtr2);
                tpgrCX2 = refBitmap->nextPixel(&tpgrCXPtr2);
                tpgrCX2 = (tpgrCX2 << 1) | refBitmap->nextPixel(&tpgrCXPtr2);
                tpgrCX2 = (tpgrCX2 << 1) | refBitmap->nextPixel(&tpgrCXPtr2);
            } else {
                tpgrCXPtr0.p = tpgrCXPtr1.p = tpgrCXPtr2.p = nullptr; // make gcc happy
                tpgrCXPtr0.shift = tpgrCXPtr1.shift = tpgrCXPtr2.shift = 0;
                tpgrCXPtr0.x = tpgrCXPtr1.x = tpgrCXPtr2.x = 0;
            }

            for (x = 0; x < w; ++x) {

                // update the context
                cx0 = ((cx0 << 1) | bitmap->nextPixel(&cxPtr0)) & 7;
                cx3 = ((cx3 << 1) | refBitmap->nextPixel(&cxPtr3)) & 7;
                cx4 = ((cx4 << 1) | refBitmap->nextPixel(&cxPtr4)) & 3;

                if (tpgrOn) {
                    // update the typical predictor context
                    tpgrCX0 = ((tpgrCX0 << 1) | refBitmap->nextPixel(&tpgrCXPtr0)) & 7;
                    tpgrCX1 = ((tpgrCX1 << 1) | refBitmap->nextPixel(&tpgrCXPtr1)) & 7;
                    tpgrCX2 = ((tpgrCX2 << 1) | refBitmap->nextPixel(&tpgrCXPtr2)) & 7;

                    // check for a "typical" pixel
                    if (arithDecoder->decodeBit(ltpCX, refinementRegionStats.get())) {
                        ltp = !ltp;
                    }
                    if (tpgrCX0 == 0 && tpgrCX1 == 0 && tpgrCX2 == 0) {
                        bitmap->clearPixel(x, y);
                        continue;
                    }
                    if (tpgrCX0 == 7 && tpgrCX1 == 7 && tpgrCX2 == 7) {
                        bitmap->setPixel(x, y);
                        continue;
                    }
                }

                // build the context
                cx = (cx0 << 7) | (bitmap->nextPixel(&cxPtr1) << 6) | (refBitmap->nextPixel(&cxPtr2) << 5) | (cx3 << 2) | cx4;

                // decode the pixel
                if ((pix = arithDecoder->decodeBit(cx, refinementRegionStats.get()))) {
                    bitmap->setPixel(x, y);
                }
                if (unlikely(arithDecoder->getReadPastEndOfStream())) {
                    return nullptr;
                }
            }

        } else {

            // set up the context
            bitmap->getPixelPtr(0, y - 1, &cxPtr0);
            cx0 = bitmap->nextPixel(&cxPtr0);
            bitmap->getPixelPtr(-1, y, &cxPtr1);
            refBitmap->getPixelPtr(-refDX, y - 1 - refDY, &cxPtr2);
            cx2 = refBitmap->nextPixel(&cxPtr2);
            refBitmap->getPixelPtr(-1 - refDX, y - refDY, &cxPtr3);
            cx3 = refBitmap->nextPixel(&cxPtr3);
            cx3 = (cx3 << 1) | refBitmap->nextPixel(&cxPtr3);
            refBitmap->getPixelPtr(-1 - refDX, y + 1 - refDY, &cxPtr4);
            cx4 = refBitmap->nextPixel(&cxPtr4);
            cx4 = (cx4 << 1) | refBitmap->nextPixel(&cxPtr4);
            bitmap->getPixelPtr(atx[0], y + aty[0], &cxPtr5);
            refBitmap->getPixelPtr(atx[1] - refDX, y + aty[1] - refDY, &cxPtr6);