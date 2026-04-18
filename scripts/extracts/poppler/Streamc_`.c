// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 32/36



    if (endOfBlock) {
        if (!startBlock()) {
            return;
        }
    }

    if (compressedBlock) {
        if ((code1 = getHuffmanCodeWord(&litCodeTab)) == EOF) {
            goto err;
        }
        if (code1 < 256) {
            buf[index] = code1;
            remain = 1;
        } else if (code1 == 256) {
            endOfBlock = true;
            remain = 0;
        } else {
            code1 -= 257;
            code2 = lengthDecode[code1].bits;
            if (code2 > 0 && (code2 = getCodeWord(code2)) == EOF) {
                goto err;
            }
            len = lengthDecode[code1].first + code2;
            if ((code1 = getHuffmanCodeWord(&distCodeTab)) == EOF) {
                goto err;
            }
            code2 = distDecode[code1].bits;
            if (code2 > 0 && (code2 = getCodeWord(code2)) == EOF) {
                goto err;
            }
            dist = distDecode[code1].first + code2;
            i = index;
            j = (index - dist) & flateMask;
            for (k = 0; k < len; ++k) {
                buf[i] = buf[j];
                i = (i + 1) & flateMask;
                j = (j + 1) & flateMask;
            }
            remain = len;
        }

    } else {
        len = (blockLen < flateWindow) ? blockLen : flateWindow;
        for (i = 0, j = index; i < len; ++i, j = (j + 1) & flateMask) {
            if ((c = str->getChar()) == EOF) {
                endOfBlock = eof = true;
                break;
            }
            buf[j] = c & 0xff;
        }
        remain = i;
        blockLen -= len;
        if (blockLen == 0) {
            endOfBlock = true;
        }
    }

    return;

err:
    error(errSyntaxError, getPos(), "Unexpected end of file in flate stream");
    endOfBlock = eof = true;
    remain = 0;
}

bool FlateStream::startBlock()
{
    int blockHdr;
    int c;
    int check;

    // free the code tables from the previous block
    if (litCodeTab.codes != fixedLitCodeTab.codes) {
        gfree(const_cast<FlateCode *>(litCodeTab.codes));
    }
    litCodeTab.codes = nullptr;
    if (distCodeTab.codes != fixedDistCodeTab.codes) {
        gfree(const_cast<FlateCode *>(distCodeTab.codes));
    }
    distCodeTab.codes = nullptr;

    // read block header
    blockHdr = getCodeWord(3);
    if (blockHdr & 1) {
        eof = true;
    }
    blockHdr >>= 1;

    // uncompressed block
    if (blockHdr == 0) {
        compressedBlock = false;
        if ((c = str->getChar()) == EOF) {
            goto err;
        }
        blockLen = c & 0xff;
        if ((c = str->getChar()) == EOF) {
            goto err;
        }
        blockLen |= (c & 0xff) << 8;
        if ((c = str->getChar()) == EOF) {
            goto err;
        }
        check = c & 0xff;
        if ((c = str->getChar()) == EOF) {
            goto err;
        }
        check |= (c & 0xff) << 8;
        if (check != (~blockLen & 0xffff)) {
            error(errSyntaxError, getPos(), "Bad uncompressed block length in flate stream");
        }
        codeBuf = 0;
        codeSize = 0;

        // compressed block with fixed codes
    } else if (blockHdr == 1) {
        compressedBlock = true;
        loadFixedCodes();

        // compressed block with dynamic codes
    } else if (blockHdr == 2) {
        compressedBlock = true;
        if (!readDynamicCodes()) {
            goto err;
        }

        // unknown block type
    } else {
        goto err;
    }

    endOfBlock = false;
    return true;

err:
    error(errSyntaxError, getPos(), "Bad block header in flate stream");
    endOfBlock = eof = true;
    return false;
}

void FlateStream::loadFixedCodes()
{
    litCodeTab.codes = fixedLitCodeTab.codes;
    litCodeTab.maxLen = fixedLitCodeTab.maxLen;
    distCodeTab.codes = fixedDistCodeTab.codes;
    distCodeTab.maxLen = fixedDistCodeTab.maxLen;
}

bool FlateStream::readDynamicCodes()
{
    int numCodeLenCodes;
    int numLitCodes;
    int numDistCodes;
    int codeLenCodeLengths[flateMaxCodeLenCodes];
    FlateHuffmanTab codeLenCodeTab;
    int len, repeat, code;
    int i;

    codeLenCodeTab.codes = nullptr;

    // read lengths
    if ((numLitCodes = getCodeWord(5)) == EOF) {
        goto err;
    }
    numLitCodes += 257;
    if ((numDistCodes = getCodeWord(5)) == EOF) {
        goto err;
    }
    numDistCodes += 1;
    if ((numCodeLenCodes = getCodeWord(4)) == EOF) {
        goto err;
    }
    numCodeLenCodes += 4;
    if (numLitCodes > flateMaxLitCodes || numDistCodes > flateMaxDistCodes || numCodeLenCodes > flateMaxCodeLenCodes) {
        goto err;
    }

    // build the code length code table
    for (i = 0; i < flateMaxCodeLenCodes; ++i) {
        codeLenCodeLengths[i] = 0;
    }
    for (i = 0; i < numCodeLenCodes; ++i) {
        if ((codeLenCodeLengths[codeLenCodeMap[i]] = getCodeWord(3)) == -1) {
            goto err;
        }
    }
    codeLenCodeTab.codes = compHuffmanCodes(codeLenCodeLengths, flateMaxCodeLenCodes, &codeLenCodeTab.maxLen);