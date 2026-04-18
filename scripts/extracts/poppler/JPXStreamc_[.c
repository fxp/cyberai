// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 27/31



// Do one level of the inverse transform:
// - take (n)LL, (n)HL, (n)LH, and (n)HH from the upper-left corner
//   of the tile-component data array
// - leave the resulting (n-1)LL in the same place
void JPXStream::inverseTransformLevel(JPXTileComp *tileComp, unsigned int r, JPXResLevel *resLevel)
{
    JPXPrecinct *precinct;
    JPXSubband *subband;
    JPXCodeBlock *cb;
    int *coeff0, *coeff;
    char *touched0, *touched;
    unsigned int qStyle, guard, eps, shift, t;
    int shift2;
    double mu;
    int val;
    int *dataPtr, *bufPtr;
    unsigned int nx1, nx2, ny1, ny2, offset;
    unsigned int x, y, sb, cbX, cbY;

    //----- fixed-point adjustment and dequantization

    qStyle = tileComp->quantStyle & 0x1f;
    guard = (tileComp->quantStyle >> 5) & 7;
    precinct = &resLevel->precincts[0];
    for (sb = 0; sb < 3; ++sb) {

        // i-quant parameters
        if (qStyle == 0) {
            cover(100);
            const unsigned int stepIndex = 3 * r - 2 + sb;
            if (unlikely(stepIndex >= tileComp->nQuantSteps)) {
                error(errSyntaxError, getPos(), "Wrong index for quantSteps in inverseTransformLevel in JPX stream");
                break;
            }
            eps = (tileComp->quantSteps[stepIndex] >> 3) & 0x1f;
            shift = guard + eps - 1;
            mu = 0; // make gcc happy
        } else {
            cover(101);
            shift = guard + tileComp->prec;
            if (sb == 2) {
                cover(102);
                ++shift;
            }
            const unsigned int stepIndex = qStyle == 1 ? 0 : (3 * r - 2 + sb);
            if (unlikely(stepIndex >= tileComp->nQuantSteps)) {
                error(errSyntaxError, getPos(), "Wrong index for quantSteps in inverseTransformLevel in JPX stream");
                break;
            }
            t = tileComp->quantSteps[stepIndex];
            mu = (double)(0x800 + (t & 0x7ff)) / 2048.0;
        }
        if (tileComp->transform == 0) {
            cover(103);
            shift += fracBits;
        }

        // fixed point adjustment and dequantization
        subband = &precinct->subbands[sb];
        cb = subband->cbs;
        for (cbY = 0; cbY < subband->nYCBs; ++cbY) {
            for (cbX = 0; cbX < subband->nXCBs; ++cbX) {
                for (y = cb->y0, coeff0 = cb->coeffs, touched0 = cb->touched; y < cb->y1; ++y, coeff0 += tileComp->w, touched0 += tileComp->cbW) {
                    for (x = cb->x0, coeff = coeff0, touched = touched0; x < cb->x1; ++x, ++coeff, ++touched) {
                        val = *coeff;
                        if (val != 0) {
                            shift2 = shift - (cb->nZeroBitPlanes + cb->len + *touched);
                            if (shift2 > 0) {
                                cover(74);
                                if (val < 0) {
                                    val = (((unsigned int)val) << shift2) - (1 << (shift2 - 1));
                                } else {
                                    val = (val << shift2) + (1 << (shift2 - 1));
                                }
                            } else {
                                cover(75);
                                val >>= -shift2;
                            }
                            if (qStyle == 0) {
                                cover(76);
                                if (tileComp->transform == 0) {
                                    val &= 0xFFFFFFFF << fracBits;
                                }
                            } else {
                                cover(77);
                                val = (int)((double)val * mu);
                            }
                        }
                        *coeff = val;
                    }
                }
                ++cb;
            }
        }
    }

    //----- inverse transform

    // compute the subband bounds:
    //    0   nx1  nx2
    //    |    |    |
    //    v    v    v
    //   +----+----+
    //   | LL | HL | <- 0
    //   +----+----+
    //   | LH | HH | <- ny1
    //   +----+----+
    //               <- ny2
    nx1 = precinct->subbands[1].x1 - precinct->subbands[1].x0;
    nx2 = nx1 + precinct->subbands[0].x1 - precinct->subbands[0].x0;
    ny1 = precinct->subbands[0].y1 - precinct->subbands[0].y0;
    ny2 = ny1 + precinct->subbands[1].y1 - precinct->subbands[1].y0;