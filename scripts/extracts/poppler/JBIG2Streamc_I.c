// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JBIG2Stream.cc
// Segment 9/38



class JBIG2Bitmap : public JBIG2Segment
{
public:
    JBIG2Bitmap(unsigned int segNumA, int wA, int hA);
    explicit JBIG2Bitmap(JBIG2Bitmap *bitmap);
    ~JBIG2Bitmap() override;
    JBIG2SegmentType getType() const override { return jbig2SegBitmap; }
    std::unique_ptr<JBIG2Bitmap> getSlice(unsigned int x, unsigned int y, unsigned int wA, unsigned int hA);
    void expand(int newH, unsigned int pixel);
    void clearToZero();
    void clearToOne();
    int getWidth() const { return w; }
    int getHeight() const { return h; }
    int getLineSize() const { return line; }
    int getPixel(int x, int y) const { return (x < 0 || x >= w || y < 0 || y >= h) ? 0 : (data[y * line + (x >> 3)] >> (7 - (x & 7))) & 1; }
    void setPixel(int x, int y) { data[y * line + (x >> 3)] |= 1 << (7 - (x & 7)); }
    void clearPixel(int x, int y) { data[y * line + (x >> 3)] &= 0x7f7f >> (x & 7); }
    void getPixelPtr(int x, int y, JBIG2BitmapPtr *ptr);
    int nextPixel(JBIG2BitmapPtr *ptr) const;
    void duplicateRow(int yDest, int ySrc);
    void combine(const JBIG2Bitmap &bitmap, int x, int y, unsigned int combOp);
    unsigned char *getDataPtr() { return data; }
    int getDataSize() const { return h * line; }
    bool isOk() const { return data != nullptr; }

private:
    int w, h, line;
    unsigned char *data;
};

JBIG2Bitmap::JBIG2Bitmap(unsigned int segNumA, int wA, int hA) : JBIG2Segment(segNumA)
{
    w = wA;
    h = hA;
    int auxW;
    if (unlikely(checkedAdd(wA, 7, &auxW))) {
        error(errSyntaxError, -1, "invalid width");
        data = nullptr;
        return;
    }
    line = auxW >> 3;

    if (w <= 0 || h <= 0 || line <= 0 || h >= (INT_MAX - 1) / line) {
        error(errSyntaxError, -1, "invalid width/height");
        data = nullptr;
        return;
    }
    // need to allocate one extra guard byte for use in combine()
    data = (unsigned char *)gmalloc_checkoverflow(h * line + 1);
    if (data != nullptr) {
        data[h * line] = 0;
    }
}

JBIG2Bitmap::JBIG2Bitmap(JBIG2Bitmap *bitmap) : JBIG2Segment(0)
{
    if (unlikely(bitmap == nullptr)) {
        error(errSyntaxError, -1, "NULL bitmap in JBIG2Bitmap");
        w = h = line = 0;
        data = nullptr;
        return;
    }

    w = bitmap->w;
    h = bitmap->h;
    line = bitmap->line;

    if (w <= 0 || h <= 0 || line <= 0 || h >= (INT_MAX - 1) / line) {
        error(errSyntaxError, -1, "invalid width/height");
        data = nullptr;
        return;
    }
    // need to allocate one extra guard byte for use in combine()
    data = (unsigned char *)gmalloc(h * line + 1);
    memcpy(data, bitmap->data, h * line);
    data[h * line] = 0;
}

JBIG2Bitmap::~JBIG2Bitmap()
{
    gfree(data);
}

//~ optimize this
std::unique_ptr<JBIG2Bitmap> JBIG2Bitmap::getSlice(unsigned int x, unsigned int y, unsigned int wA, unsigned int hA)
{
    if (!data) {
        return {};
    }

    auto slice = std::make_unique<JBIG2Bitmap>(0, wA, hA);
    if (!slice->isOk()) {
        return {};
    }

    slice->clearToZero();
    for (unsigned int yy = 0; yy < hA; ++yy) {
        for (unsigned int xx = 0; xx < wA; ++xx) {
            if (getPixel(x + xx, y + yy)) {
                slice->setPixel(xx, yy);
            }
        }
    }
    return slice;
}

void JBIG2Bitmap::expand(int newH, unsigned int pixel)
{
    if (unlikely(!data)) {
        return;
    }
    if (newH <= h || line <= 0 || newH >= (INT_MAX - 1) / line) {
        error(errSyntaxError, -1, "invalid width/height");
        gfree(data);
        data = nullptr;
        return;
    }
    // need to allocate one extra guard byte for use in combine()
    data = (unsigned char *)grealloc(data, newH * line + 1);
    if (pixel) {
        memset(data + h * line, 0xff, (newH - h) * line);
    } else {
        memset(data + h * line, 0x00, (newH - h) * line);
    }
    h = newH;
    data[h * line] = 0;
}

void JBIG2Bitmap::clearToZero()
{
    memset(data, 0, h * line);
}

void JBIG2Bitmap::clearToOne()
{
    memset(data, 0xff, h * line);
}

inline void JBIG2Bitmap::getPixelPtr(int x, int y, JBIG2BitmapPtr *ptr)
{
    if (y < 0 || y >= h || x >= w) {
        ptr->p = nullptr;
        ptr->shift = 0; // make gcc happy
        ptr->x = 0; // make gcc happy
    } else if (x < 0) {
        ptr->p = &data[y * line];
        ptr->shift = 7;
        ptr->x = x;
    } else {
        ptr->p = &data[y * line + (x >> 3)];
        ptr->shift = 7 - (x & 7);
        ptr->x = x;
    }
}

inline int JBIG2Bitmap::nextPixel(JBIG2BitmapPtr *ptr) const
{
    int pix;

    if (!ptr->p) {
        pix = 0;
    } else if (ptr->x < 0) {
        ++ptr->x;
        pix = 0;
    } else {
        pix = (*ptr->p >> ptr->shift) & 1;
        if (++ptr->x == w) {
            ptr->p = nullptr;
        } else if (ptr->shift == 0) {
            ++ptr->p;
            ptr->shift = 7;
        } else {
            --ptr->shift;
        }
    }
    return pix;
}