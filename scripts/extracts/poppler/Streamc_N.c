// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 14/36

e {0:04x} in CCITTFax stream", code1);
                    addPixels(columns, 0);
                    err = true;
                    break;
                }
            }

            // 1-D encoding
        } else {
            codingLine[0] = 0;
            a0i = 0;
            blackPixels = 0;
            while (codingLine[a0i] < columns) {
                code1 = 0;
                if (blackPixels) {
                    do {
                        code1 += code3 = getBlackCode();
                    } while (code3 >= 64);
                } else {
                    do {
                        code1 += code3 = getWhiteCode();
                    } while (code3 >= 64);
                }
                addPixels(codingLine[a0i] + code1, blackPixels);
                blackPixels ^= 1;
            }
        }

        // check for end-of-line marker, skipping over any extra zero bits
        // (if EncodedByteAlign is true and EndOfLine is false, there can
        // be "false" EOL markers -- i.e., if the last n unused bits in
        // row i are set to zero, and the first 11-n bits in row i+1
        // happen to be zero -- so we don't look for EOL markers in this
        // case)
        gotEOL = false;
        if (!endOfBlock && row == rows - 1) {
            eof = true;
        } else if (endOfLine || !byteAlign) {
            code1 = lookBits(12);
            if (endOfLine) {
                while (code1 != EOF && code1 != 0x001) {
                    eatBits(1);
                    code1 = lookBits(12);
                }
            } else {
                while (code1 == 0) {
                    eatBits(1);
                    code1 = lookBits(12);
                }
            }
            if (code1 == 0x001) {
                eatBits(12);
                gotEOL = true;
            }
        }

        // byte-align the row
        // (Adobe apparently doesn't do byte alignment after EOL markers
        // -- I've seen CCITT image data streams in two different formats,
        // both with the byteAlign flag set:
        //   1. xx:x0:01:yy:yy
        //   2. xx:00:1y:yy:yy
        // where xx is the previous line, yy is the next line, and colons
        // separate bytes.)
        if (byteAlign && !gotEOL) {
            inputBits &= ~7;
        }

        // check for end of stream
        if (lookBits(1) == EOF) {
            eof = true;
        }

        // get 2D encoding tag
        if (!eof && encoding > 0) {
            nextLine2D = !lookBits(1);
            eatBits(1);
        }

        // check for end-of-block marker
        if (endOfBlock && !endOfLine && byteAlign) {
            // in this case, we didn't check for an EOL code above, so we
            // need to check here
            code1 = lookBits(24);
            if (code1 == 0x001001) {
                eatBits(12);
                gotEOL = true;
            }
        }
        if (endOfBlock && gotEOL) {
            code1 = lookBits(12);
            if (code1 == 0x001) {
                eatBits(12);
                if (encoding > 0) {
                    lookBits(1);
                    eatBits(1);
                }
                if (encoding >= 0) {
                    for (i = 0; i < 4; ++i) {
                        code1 = lookBits(12);
                        if (code1 != 0x001) {
                            error(errSyntaxError, getPos(), "Bad RTC code in CCITTFax stream");
                        }
                        eatBits(12);
                        if (encoding > 0) {
                            lookBits(1);
                            eatBits(1);
                        }
                    }
                }
                eof = true;
            }

            // look for an end-of-line marker after an error -- we only do
            // this if we know the stream contains end-of-line markers because
            // the "just plow on" technique tends to work better otherwise
        } else if (err && endOfLine) {
            while (true) {
                code1 = lookBits(13);
                if (code1 == EOF) {
                    eof = true;
                    return EOF;
                }
                if ((code1 >> 1) == 0x001) {
                    break;
                }
                eatBits(1);
            }
            eatBits(12);
            if (encoding > 0) {
                eatBits(1);
                nextLine2D = !(code1 & 1);
            }
        }

        // set up for output
        if (codingLine[0] > 0) {
            outputBits = codingLine[a0i = 0];
        } else {
            outputBits = codingLine[a0i = 1];
        }

        ++row;
    }