// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 33/36



    // build the literal and distance code tables
    len = 0;
    repeat = 0;
    i = 0;
    while (i < numLitCodes + numDistCodes) {
        if ((code = getHuffmanCodeWord(&codeLenCodeTab)) == EOF) {
            goto err;
        }
        if (code == 16) {
            if ((repeat = getCodeWord(2)) == EOF) {
                goto err;
            }
            repeat += 3;
            if (i + repeat > numLitCodes + numDistCodes) {
                goto err;
            }
            for (; repeat > 0; --repeat) {
                codeLengths[i++] = len;
            }
        } else if (code == 17) {
            if ((repeat = getCodeWord(3)) == EOF) {
                goto err;
            }
            repeat += 3;
            if (i + repeat > numLitCodes + numDistCodes) {
                goto err;
            }
            len = 0;
            for (; repeat > 0; --repeat) {
                codeLengths[i++] = 0;
            }
        } else if (code == 18) {
            if ((repeat = getCodeWord(7)) == EOF) {
                goto err;
            }
            repeat += 11;
            if (i + repeat > numLitCodes + numDistCodes) {
                goto err;
            }
            len = 0;
            for (; repeat > 0; --repeat) {
                codeLengths[i++] = 0;
            }
        } else {
            codeLengths[i++] = len = code;
        }
    }
    litCodeTab.codes = compHuffmanCodes(codeLengths, numLitCodes, &litCodeTab.maxLen);
    distCodeTab.codes = compHuffmanCodes(codeLengths + numLitCodes, numDistCodes, &distCodeTab.maxLen);

    gfree(const_cast<FlateCode *>(codeLenCodeTab.codes));
    return true;

err:
    error(errSyntaxError, getPos(), "Bad dynamic code table in flate stream");
    gfree(const_cast<FlateCode *>(codeLenCodeTab.codes));
    return false;
}

// Convert an array <lengths> of <n> lengths, in value order, into a
// Huffman code lookup table.
FlateCode *FlateStream::compHuffmanCodes(const int *lengths, int n, int *maxLen)
{
    int len, code, code2, skip, val, i, t;

    // find max code length
    *maxLen = 0;
    for (val = 0; val < n; ++val) {
        if (lengths[val] > *maxLen) {
            *maxLen = lengths[val];
        }
    }

    // allocate the table
    const int tabSize = 1 << *maxLen;
    auto *codes = (FlateCode *)gmallocn(tabSize, sizeof(FlateCode));

    // clear the table
    for (i = 0; i < tabSize; ++i) {
        codes[i].len = 0;
        codes[i].val = 0;
    }

    // build the table
    for (len = 1, code = 0, skip = 2; len <= *maxLen; ++len, code <<= 1, skip <<= 1) {
        for (val = 0; val < n; ++val) {
            if (lengths[val] == len) {

                // bit-reverse the code
                code2 = 0;
                t = code;
                for (i = 0; i < len; ++i) {
                    code2 = (code2 << 1) | (t & 1);
                    t >>= 1;
                }

                // fill in the table entries
                for (i = code2; i < tabSize; i += skip) {
                    codes[i].len = (unsigned short)len;
                    codes[i].val = (unsigned short)val;
                }

                ++code;
            }
        }
    }

    return codes;
}

int FlateStream::getHuffmanCodeWord(FlateHuffmanTab *tab)
{
    const FlateCode *code;
    int c;

    while (codeSize < tab->maxLen) {
        if ((c = str->getChar()) == EOF) {
            break;
        }
        codeBuf |= (c & 0xff) << codeSize;
        codeSize += 8;
    }
    code = &tab->codes[codeBuf & ((1 << tab->maxLen) - 1)];
    if (codeSize == 0 || codeSize < code->len || code->len == 0) {
        return EOF;
    }
    codeBuf >>= code->len;
    codeSize -= code->len;
    return (int)code->val;
}

int FlateStream::getCodeWord(int bits)
{
    int c;

    while (codeSize < bits) {
        if ((c = str->getChar()) == EOF) {
            return EOF;
        }
        codeBuf |= (c & 0xff) << codeSize;
        codeSize += 8;
    }
    c = codeBuf & ((1 << bits) - 1);
    codeBuf >>= bits;
    codeSize -= bits;
    return c;
}
#endif

//------------------------------------------------------------------------
// EOFStream
//------------------------------------------------------------------------

EOFStream::EOFStream(std::unique_ptr<Stream> strA) : OwnedFilterStream(std::move(strA)) { }

EOFStream::~EOFStream() = default;

//------------------------------------------------------------------------
// BufStream
//------------------------------------------------------------------------

BufStream::BufStream(std::unique_ptr<Stream> strA, int bufSizeA) : OwnedFilterStream(std::move(strA))
{
    bufSize = bufSizeA;
    buf = (int *)gmallocn(bufSize, sizeof(int));
}

BufStream::~BufStream()
{
    gfree(buf);
}

bool BufStream::rewind()
{
    int i;

    bool success = str->rewind();
    for (i = 0; i < bufSize; ++i) {
        buf[i] = str->getChar();
    }

    return success;
}

int BufStream::getChar()
{
    int c, i;