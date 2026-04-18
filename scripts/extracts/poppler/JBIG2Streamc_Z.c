// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 26/38



inline void JBIG2Stream::mmrAddPixels(int a1, int blackPixels, int *codingLine, int *a0i, int w)
{
    if (a1 > codingLine[*a0i]) {
        if (a1 > w) {
            error(errSyntaxError, curStr->getPos(), "JBIG2 MMR row is wrong length ({0:d})", a1);
            a1 = w;
        }
        if ((*a0i & 1) ^ blackPixels) {
            ++*a0i;
        }
        codingLine[*a0i] = a1;
    }
}

inline void JBIG2Stream::mmrAddPixelsNeg(int a1, int blackPixels, int *codingLine, int *a0i, int w)
{
    if (a1 > codingLine[*a0i]) {
        if (a1 > w) {
            error(errSyntaxError, curStr->getPos(), "JBIG2 MMR row is wrong length ({0:d})", a1);
            a1 = w;
        }
        if ((*a0i & 1) ^ blackPixels) {
            ++*a0i;
        }
        codingLine[*a0i] = a1;
    } else if (a1 < codingLine[*a0i]) {
        if (a1 < 0) {
            error(errSyntaxError, curStr->getPos(), "Invalid JBIG2 MMR code");
            a1 = 0;
        }
        while (*a0i > 0 && a1 <= codingLine[*a0i - 1]) {
            --*a0i;
        }
        codingLine[*a0i] = a1;
    }
}

static inline bool isPixelOutsideAdaptiveField(int x, int y)
{
    return y < -128 || y > 0 || x < -128 || (y < 0 && x > 127) || (y == 0 && x >= 0);
}

std::unique_ptr<JBIG2Bitmap> JBIG2Stream::readGenericBitmap(bool mmr, int w, int h, int templ, bool tpgdOn, bool useSkip, JBIG2Bitmap *skip, int *atx, int *aty, int mmrDataLength)
{
    auto bitmap = std::make_unique<JBIG2Bitmap>(0, w, h);
    if (!bitmap->isOk()) {
        return nullptr;
    }
    bitmap->clearToZero();

    //----- MMR decode

    if (mmr) {

        mmrDecoder->reset();
        // 0 <= codingLine[0] < codingLine[1] < ... < codingLine[n] = w
        // ---> max codingLine size = w + 1
        // refLine has one extra guard entry at the end
        // ---> max refLine size = w + 2
        int *codingLine = (int *)gmallocn_checkoverflow(w + 1, sizeof(int));
        int *refLine = (int *)gmallocn_checkoverflow(w + 2, sizeof(int));

        if (unlikely(!codingLine || !refLine)) {
            gfree(codingLine);
            error(errSyntaxError, curStr->getPos(), "Bad width in JBIG2 generic bitmap");
            return nullptr;
        }

        memset(refLine, 0, (w + 2) * sizeof(int));
        for (int i = 0; i < w + 1; ++i) {
            codingLine[i] = w;
        }

        for (int y = 0; y < h; ++y) {

            // copy coding line to ref line
            {
                int i;
                for (i = 0; codingLine[i] < w; ++i) {
                    refLine[i] = codingLine[i];
                }
                refLine[i++] = w;
                refLine[i] = w;
            }