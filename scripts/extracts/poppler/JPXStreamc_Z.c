// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 26/31

1;
                                } else {
                                    *coeff = 1;
                                }
                            }
                        } else {
                            *touched = 0;
                        }
                    }
                }
            }
            ++cb->len;
            // look for a segmentation symbol
            if (tileComp->codeBlockStyle & 0x20) {
                segSym = cb->arithDecoder->decodeBit(jpxContextUniform, cb->stats) << 3;
                segSym |= cb->arithDecoder->decodeBit(jpxContextUniform, cb->stats) << 2;
                segSym |= cb->arithDecoder->decodeBit(jpxContextUniform, cb->stats) << 1;
                segSym |= cb->arithDecoder->decodeBit(jpxContextUniform, cb->stats);
                if (segSym != 0x0a) {
                    // in theory this should be a fatal error, but it seems to
                    // be problematic
                    error(errSyntaxWarning, getPos(), "Missing or invalid segmentation symbol in JPX stream");
                }
            }
            cb->nextPass = jpxPassSigProp;
            break;
        }

        if (tileComp->codeBlockStyle & 0x02) {
            cb->stats->resetContext();
            cb->stats->setEntry(jpxContextSigProp, 4, 0);
            cb->stats->setEntry(jpxContextRunLength, 3, 0);
            cb->stats->setEntry(jpxContextUniform, 46, 0);
        }

        if (tileComp->codeBlockStyle & 0x04) {
            cb->arithDecoder->cleanup();
        }
    }

    cb->arithDecoder->cleanup();
    return true;
}

// Inverse quantization, and wavelet transform (IDWT).  This also does
// the initial shift to convert to fixed point format.
void JPXStream::inverseTransform(JPXTileComp *tileComp)
{
    JPXResLevel *resLevel;
    JPXPrecinct *precinct;
    JPXSubband *subband;
    JPXCodeBlock *cb;
    int *coeff0, *coeff;
    char *touched0, *touched;
    unsigned int qStyle, guard, eps, shift;
    int shift2;
    double mu;
    int val;
    unsigned int r, cbX, cbY, x, y;

    cover(68);

    //----- (NL)LL subband (resolution level 0)

    resLevel = &tileComp->resLevels[0];
    precinct = &resLevel->precincts[0];
    subband = &precinct->subbands[0];

    // i-quant parameters
    qStyle = tileComp->quantStyle & 0x1f;
    guard = (tileComp->quantStyle >> 5) & 7;
    if (qStyle == 0) {
        cover(69);
        eps = (tileComp->quantSteps[0] >> 3) & 0x1f;
        shift = guard + eps - 1;
        mu = 0; // make gcc happy
    } else {
        cover(70);
        shift = guard - 1 + tileComp->prec;
        mu = (double)(0x800 + (tileComp->quantSteps[0] & 0x7ff)) / 2048.0;
    }
    if (tileComp->transform == 0) {
        cover(71);
        shift += fracBits;
    }

    // do fixed point adjustment and dequantization on (NL)LL
    cb = subband->cbs;
    for (cbY = 0; cbY < subband->nYCBs; ++cbY) {
        for (cbX = 0; cbX < subband->nXCBs; ++cbX) {
            for (y = cb->y0, coeff0 = cb->coeffs, touched0 = cb->touched; y < cb->y1; ++y, coeff0 += tileComp->w, touched0 += tileComp->cbW) {
                for (x = cb->x0, coeff = coeff0, touched = touched0; x < cb->x1; ++x, ++coeff, ++touched) {
                    val = *coeff;
                    if (val != 0) {
                        shift2 = shift - (cb->nZeroBitPlanes + cb->len + *touched);
                        if (shift2 > 0) {
                            cover(94);
                            if (val < 0) {
                                val = (((unsigned int)val) << shift2) - (1 << (shift2 - 1));
                            } else {
                                val = (val << shift2) + (1 << (shift2 - 1));
                            }
                        } else {
                            cover(95);
                            val >>= -shift2;
                        }
                        if (qStyle == 0) {
                            cover(96);
                            if (tileComp->transform == 0) {
                                cover(97);
                                val &= 0xFFFFFFFF << fracBits;
                            }
                        } else {
                            cover(98);
                            val = (int)((double)val * mu);
                        }
                    }
                    *coeff = val;
                }
            }
            ++cb;
        }
    }

    //----- IDWT for each level

    for (r = 1; r <= tileComp->nDecompLevels; ++r) {
        resLevel = &tileComp->resLevels[r];

        // (n)LL is already in the upper-left corner of the
        // tile-component data array -- interleave with (n)HL/LH/HH
        // and inverse transform to get (n-1)LL, which will be stored
        // in the upper-left corner of the tile-component data array
        inverseTransformLevel(tileComp, r, resLevel);
    }
}