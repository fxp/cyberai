// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 2/31



// arithmetic decoder context for the significance propagation and
// cleanup passes:
//     [horiz][vert][diag][subband]
// where subband = 0 for HL
//               = 1 for LH and LL
//               = 2 for HH
static const unsigned int sigPropContext[3][3][5][3] = {
    { { { 0, 0, 0 }, // horiz=0, vert=0, diag=0
        { 1, 1, 3 }, // horiz=0, vert=0, diag=1
        { 2, 2, 6 }, // horiz=0, vert=0, diag=2
        { 2, 2, 8 }, // horiz=0, vert=0, diag=3
        { 2, 2, 8 } }, // horiz=0, vert=0, diag=4
      { { 5, 3, 1 }, // horiz=0, vert=1, diag=0
        { 6, 3, 4 }, // horiz=0, vert=1, diag=1
        { 6, 3, 7 }, // horiz=0, vert=1, diag=2
        { 6, 3, 8 }, // horiz=0, vert=1, diag=3
        { 6, 3, 8 } }, // horiz=0, vert=1, diag=4
      { { 8, 4, 2 }, // horiz=0, vert=2, diag=0
        { 8, 4, 5 }, // horiz=0, vert=2, diag=1
        { 8, 4, 7 }, // horiz=0, vert=2, diag=2
        { 8, 4, 8 }, // horiz=0, vert=2, diag=3
        { 8, 4, 8 } } }, // horiz=0, vert=2, diag=4
    { { { 3, 5, 1 }, // horiz=1, vert=0, diag=0
        { 3, 6, 4 }, // horiz=1, vert=0, diag=1
        { 3, 6, 7 }, // horiz=1, vert=0, diag=2
        { 3, 6, 8 }, // horiz=1, vert=0, diag=3
        { 3, 6, 8 } }, // horiz=1, vert=0, diag=4
      { { 7, 7, 2 }, // horiz=1, vert=1, diag=0
        { 7, 7, 5 }, // horiz=1, vert=1, diag=1
        { 7, 7, 7 }, // horiz=1, vert=1, diag=2
        { 7, 7, 8 }, // horiz=1, vert=1, diag=3
        { 7, 7, 8 } }, // horiz=1, vert=1, diag=4
      { { 8, 7, 2 }, // horiz=1, vert=2, diag=0
        { 8, 7, 5 }, // horiz=1, vert=2, diag=1
        { 8, 7, 7 }, // horiz=1, vert=2, diag=2
        { 8, 7, 8 }, // horiz=1, vert=2, diag=3
        { 8, 7, 8 } } }, // horiz=1, vert=2, diag=4
    { { { 4, 8, 2 }, // horiz=2, vert=0, diag=0
        { 4, 8, 5 }, // horiz=2, vert=0, diag=1
        { 4, 8, 7 }, // horiz=2, vert=0, diag=2
        { 4, 8, 8 }, // horiz=2, vert=0, diag=3
        { 4, 8, 8 } }, // horiz=2, vert=0, diag=4
      { { 7, 8, 2 }, // horiz=2, vert=1, diag=0
        { 7, 8, 5 }, // horiz=2, vert=1, diag=1
        { 7, 8, 7 }, // horiz=2, vert=1, diag=2
        { 7, 8, 8 }, // horiz=2, vert=1, diag=3
        { 7, 8, 8 } }, // horiz=2, vert=1, diag=4
      { { 8, 8, 2 }, // horiz=2, vert=2, diag=0
        { 8, 8, 5 }, // horiz=2, vert=2, diag=1
        { 8, 8, 7 }, // horiz=2, vert=2, diag=2
        { 8, 8, 8 }, // horiz=2, vert=2, diag=3
        { 8, 8, 8 } } } // horiz=2, vert=2, diag=4
};

// arithmetic decoder context and xor bit for the sign bit in the
// significance propagation pass:
//     [horiz][vert][k]
// where horiz/vert are offset by 2 (i.e., range is -2 .. 2)
// and k = 0 for the context
//       = 1 for the xor bit
static const unsigned int signContext[5][5][2] = {
    { { 13, 1 }, // horiz=-2, vert=-2
      { 13, 1 }, // horiz=-2, vert=-1
      { 12, 1 }, // horiz=-2, vert= 0
      { 11, 1 }, // horiz=-2, vert=+1
      { 11, 1 } }, // horiz=-2, vert=+2
    { { 13, 1 }, // horiz=-1, vert=-2
      { 13, 1 }, // horiz=-1, vert=-1
      { 12, 1 }, // horiz=-1, vert= 0
      { 11, 1 }, // horiz=-1, vert=+1
      { 11, 1 } }, // horiz=-1, vert=+2
    { { 10, 1 }, // horiz= 0, vert=-2
      { 10, 1 }, // horiz= 0, vert=-1
      { 9, 0 }, // horiz= 0, vert= 0
      { 10, 0 }, // horiz= 0, vert=+1
      { 10, 0 } }, // horiz= 0, vert=+2
    { { 11, 0 }, // horiz=+1, vert=-2
      { 11, 0 }, // horiz=+1, vert=-1
      { 12, 0 }, // horiz=+1, vert= 0
      { 13, 0 }, // horiz=+1, vert=+1
      { 13, 0 } }, // horiz=+1, vert=+2
    { { 11, 0 }, // horiz=+2, vert=-2
      { 11, 0 }, // horiz=+2, vert=-1
      { 12, 0 }, // horiz=+2, vert= 0
      { 13, 0 }, // horiz=+2, vert=+1
      { 13, 0 } }, // horiz=+2, vert=+2
};

//------------------------------------------------------------------------

// constants used in the IDWT
#define idwtAlpha -1.586134342059924
#define idwtBeta -0.052980118572961
#define idwtGamma 0.882911075530934
#define idwtDelta 0.443506852043971
#define idwtKappa 1.230174104914001
#define idwtIKappa (1.0 / idwtKappa)

// number of bits to the right of the decimal point for the fixed
// point arithmetic used in the IDWT
#define fracBits 16

//------------------------------------------------------------------------

// floor(x / y)
#define jpxFloorDiv(x, y) ((x) / (y))

// floor(x / 2^y)
#define jpxFloorDivPow2(x, y) ((x) >> (y))

// ceil(x / y)
#define jpxCeilDiv(x, y) (((x) + (y) - 1) / (y))

// ceil(x / 2^y)
#define jpxCeilDivPow2(x, y) (((x) + (1 << (y)) - 1) >> (y))

//------------------------------------------------------------------------

#if 1 //----- disable coverage tracking

#    define cover(idx)

#else //----- enable coverage tracking

class JPXCover
{
public:
    JPXCover(int sizeA);
    ~JPXCover();
    void incr(int idx);

private:
    int size, used;
    int *data;
};

JPXCover::JPXCover(int sizeA)
{
    size = sizeA;
    used = -1;
    data = (int *)gmallocn(size, sizeof(int));
    memset(data, 0, size * sizeof(int));
}