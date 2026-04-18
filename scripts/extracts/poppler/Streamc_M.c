// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Stream.cc
// Segment 13/36

           ++b1i;
                        while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
                            b1i += 2;
                            if (unlikely(b1i > columns + 1)) {
                                error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                                err = true;
                                break;
                            }
                        }
                    }
                    break;
                case twoDimVert0:
                    if (unlikely(b1i > columns + 1)) {
                        error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                        err = true;
                        break;
                    }
                    addPixels(refLine[b1i], blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < columns) {
                        ++b1i;
                        while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
                            b1i += 2;
                            if (unlikely(b1i > columns + 1)) {
                                error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                                err = true;
                                break;
                            }
                        }
                    }
                    break;
                case twoDimVertL3:
                    if (unlikely(b1i > columns + 1)) {
                        error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                        err = true;
                        break;
                    }
                    addPixelsNeg(refLine[b1i] - 3, blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < columns) {
                        if (b1i > 0) {
                            --b1i;
                        } else {
                            ++b1i;
                        }
                        while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
                            b1i += 2;
                            if (unlikely(b1i > columns + 1)) {
                                error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                                err = true;
                                break;
                            }
                        }
                    }
                    break;
                case twoDimVertL2:
                    if (unlikely(b1i > columns + 1)) {
                        error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                        err = true;
                        break;
                    }
                    addPixelsNeg(refLine[b1i] - 2, blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < columns) {
                        if (b1i > 0) {
                            --b1i;
                        } else {
                            ++b1i;
                        }
                        while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
                            b1i += 2;
                            if (unlikely(b1i > columns + 1)) {
                                error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                                err = true;
                                break;
                            }
                        }
                    }
                    break;
                case twoDimVertL1:
                    if (unlikely(b1i > columns + 1)) {
                        error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                        err = true;
                        break;
                    }
                    addPixelsNeg(refLine[b1i] - 1, blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < columns) {
                        if (b1i > 0) {
                            --b1i;
                        } else {
                            ++b1i;
                        }
                        while (refLine[b1i] <= codingLine[a0i] && refLine[b1i] < columns) {
                            b1i += 2;
                            if (unlikely(b1i > columns + 1)) {
                                error(errSyntaxError, getPos(), "Bad 2D code {0:04x} in CCITTFax stream", code1);
                                err = true;
                                break;
                            }
                        }
                    }
                    break;
                case EOF:
                    addPixels(columns, 0);
                    eof = true;
                    break;
                default:
                    error(errSyntaxError, getPos(), "Bad 2D cod