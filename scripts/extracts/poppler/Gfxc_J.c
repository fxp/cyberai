// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Gfx.cc
// Segment 10/41



    // soft mask
    obj2 = obj1.dictLookup("SMask");
    if (!obj2.isNull()) {
        if (obj2.isName("None")) {
            out->clearSoftMask(state);
        } else if (obj2.isDict()) {
            Object obj3 = obj2.dictLookup("S");
            alpha = obj3.isName("Alpha");
            std::unique_ptr<Function> softMaskTransferFunc = nullptr;
            obj3 = obj2.dictLookup("TR");
            if (!obj3.isNull()) {
                if (obj3.isName("Default") || obj3.isName("Identity")) {
                    // nothing
                } else {
                    softMaskTransferFunc = Function::parse(&obj3);
                    if (softMaskTransferFunc == nullptr || softMaskTransferFunc->getInputSize() != 1 || softMaskTransferFunc->getOutputSize() != 1) {
                        error(errSyntaxError, getPos(), "Invalid transfer function in soft mask in ExtGState");
                        softMaskTransferFunc.reset();
                    }
                }
            }
            obj3 = obj2.dictLookup("BC");
            if ((haveBackdropColor = obj3.isArray())) {
                for (int &c : backdropColor.c) {
                    c = 0;
                }
                for (int i = 0; i < obj3.arrayGetLength() && i < gfxColorMaxComps; ++i) {
                    Object obj4 = obj3.arrayGet(i);
                    if (obj4.isNum()) {
                        backdropColor.c[i] = dblToCol(obj4.getNum());
                    }
                }
            }
            obj3 = obj2.dictLookup("G");
            if (obj3.isStream()) {
                Object obj4 = obj3.streamGetDict()->lookup("Group");
                if (obj4.isDict()) {
                    std::unique_ptr<GfxColorSpace> blendingColorSpace;
                    Object obj5 = obj4.dictLookup("CS");
                    if (!obj5.isNull()) {
                        blendingColorSpace = GfxColorSpace::parse(res, &obj5, out, state);
                    }
                    const bool isolated = obj4.dictLookup("I").getBoolWithDefaultValue(false);
                    const bool knockout = obj4.dictLookup("K").getBoolWithDefaultValue(false);
                    if (!haveBackdropColor) {
                        if (blendingColorSpace) {
                            blendingColorSpace->getDefaultColor(&backdropColor);
                        } else {
                            //~ need to get the parent or default color space (?)
                            for (int &c : backdropColor.c) {
                                c = 0;
                            }
                        }
                    }
                    doSoftMask(&obj3, alpha, blendingColorSpace.get(), isolated, knockout, softMaskTransferFunc.get(), &backdropColor);
                } else {
                    error(errSyntaxError, getPos(), "Invalid soft mask in ExtGState - missing group");
                }
            } else {
                error(errSyntaxError, getPos(), "Invalid soft mask in ExtGState - missing group");
            }
        } else if (!obj2.isNull()) {
            error(errSyntaxError, getPos(), "Invalid soft mask in ExtGState");
        }
    }
    obj2 = obj1.dictLookup("Font");
    if (obj2.isArray()) {
        if (obj2.arrayGetLength() == 2) {
            const Object &fargs0 = obj2.arrayGetNF(0);
            Object fargs1 = obj2.arrayGet(1);
            if (fargs0.isRef() && fargs1.isNum()) {
                Object fobj = fargs0.fetch(xref);
                if (fobj.isDict()) {
                    Ref r = fargs0.getRef();
                    std::shared_ptr<GfxFont> font = GfxFont::makeFont(xref, args[0].getName(), r, *fobj.getDict());
                    state->setFont(font, fargs1.getNum());
                    fontChanged = true;
                }
            }
        } else {
            error(errSyntaxError, getPos(), "Number of args mismatch for /Font in ExtGState");
        }
    }
    obj2 = obj1.dictLookup("LW");
    if (obj2.isNum()) {
        opSetLineWidth(&obj2, 1);
    }
    obj2 = obj1.dictLookup("LC");
    if (obj2.isInt()) {
        opSetLineCap(&obj2, 1);
    }
    obj2 = obj1.dictLookup("LJ");
    if (obj2.isInt()) {
        opSetLineJoin(&obj2, 1);
    }
    obj2 = obj1.dictLookup("ML");
    if (obj2.isNum()) {
        opSetMiterLimit(&obj2, 1);
    }
    obj2 = obj1.dictLookup("D");
    if (obj2.isArray()) {
        if (obj2.arrayGetLength() == 2) {
            Object dargs[2];

            dargs[0] = obj2.arrayGetNF(0).copy();
            dargs[1] = obj2.arrayGet(1);
            if (dargs[0].isArray() && dargs[1].isInt()) {
                opSetDash(dargs, 2);
            }
        } else {
            error(errSyntaxError, getPos(), "Number of args mismatch for /D in ExtGState");
        }
    }
    obj2 = obj1.dictLookup("RI");
    if (obj2.isName()) {
        opSetRenderingIntent(&obj2, 1);
    }
    obj2 = obj1.dictLookup("FL");
    if (obj2.isNum()) {
        opSetFlat(&obj2, 1);
    }
}