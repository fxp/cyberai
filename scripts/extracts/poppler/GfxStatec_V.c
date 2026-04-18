// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 22/49



void GfxDeviceNColorSpace::createMapping(std::vector<std::unique_ptr<GfxSeparationColorSpace>> *separationList, size_t maxSepComps)
{
    if (nonMarking) { // None
        return;
    }
    mapping.resize(nComps);
    unsigned int newOverprintMask = 0;
    for (int i = 0; i < nComps; i++) {
        if (names[i] == "None") {
            mapping[i] = -1;
        } else if (names[i] == "Cyan") {
            newOverprintMask |= 0x01;
            mapping[i] = 0;
        } else if (names[i] == "Magenta") {
            newOverprintMask |= 0x02;
            mapping[i] = 1;
        } else if (names[i] == "Yellow") {
            newOverprintMask |= 0x04;
            mapping[i] = 2;
        } else if (names[i] == "Black") {
            newOverprintMask |= 0x08;
            mapping[i] = 3;
        } else {
            unsigned int startOverprintMask = 0x10;
            bool found = false;
            const Function *sepFunc = nullptr;
            if (nComps == 1) {
                sepFunc = func.get();
            } else {
                for (const std::unique_ptr<GfxSeparationColorSpace> &sepCS : sepsCS) {
                    if (!sepCS->getName()->compare(names[i])) {
                        sepFunc = sepCS->getFunc();
                        break;
                    }
                }
            }
            for (std::size_t j = 0; j < separationList->size(); j++) {
                const std::unique_ptr<GfxSeparationColorSpace> &sepCS = (*separationList)[j];
                if (!sepCS->getName()->compare(names[i])) {
                    if (sepFunc != nullptr && sepCS->getFunc()->hasDifferentResultSet(sepFunc)) {
                        error(errSyntaxWarning, -1, "Different functions found for '{0:s}', convert immediately", names[i].c_str());
                        mapping.clear();
                        overprintMask = 0xffffffff;
                        return;
                    }
                    mapping[i] = j + 4;
                    newOverprintMask |= startOverprintMask;
                    found = true;
                    break;
                }
                startOverprintMask <<= 1;
            }
            if (!found) {
                if (separationList->size() == maxSepComps) {
                    error(errSyntaxWarning, -1, "Too many ({0:ulld}) spots, convert '{1:s}' immediately", static_cast<unsigned long long>(maxSepComps), names[i].c_str());
                    mapping.clear();
                    overprintMask = 0xffffffff;
                    return;
                }
                mapping[i] = separationList->size() + 4;
                newOverprintMask |= startOverprintMask;
                if (nComps == 1) {
                    separationList->push_back(std::make_unique<GfxSeparationColorSpace>(std::make_unique<GooString>(names[i]), alt->copy(), func->copy()));
                } else {
                    for (const std::unique_ptr<GfxSeparationColorSpace> &sepCS : sepsCS) {
                        if (!sepCS->getName()->compare(names[i])) {
                            found = true;
                            separationList->push_back(sepCS->copyAsOwnType());
                            break;
                        }
                    }
                    if (!found) {
                        error(errSyntaxWarning, -1, "DeviceN has no suitable colorant");
                        mapping.clear();
                        overprintMask = 0xffffffff;
                        return;
                    }
                }
            }
        }
    }
    overprintMask = newOverprintMask;
}

//------------------------------------------------------------------------
// GfxPatternColorSpace
//------------------------------------------------------------------------

GfxPatternColorSpace::GfxPatternColorSpace(std::unique_ptr<GfxColorSpace> &&underA) : under(std::move(underA)) { }

GfxPatternColorSpace::~GfxPatternColorSpace() = default;

std::unique_ptr<GfxColorSpace> GfxPatternColorSpace::copy() const
{
    return std::make_unique<GfxPatternColorSpace>(under ? under->copy() : nullptr);
}

std::unique_ptr<GfxColorSpace> GfxPatternColorSpace::parse(GfxResources *res, const Array &arr, OutputDev *out, GfxState *state, int recursion)
{
    Object obj1;

    if (arr.getLength() != 1 && arr.getLength() != 2) {
        error(errSyntaxWarning, -1, "Bad Pattern color space");
        return {};
    }
    std::unique_ptr<GfxColorSpace> underA;
    if (arr.getLength() == 2) {
        obj1 = arr.get(1);
        if (!(underA = GfxColorSpace::parse(res, &obj1, out, state, recursion + 1))) {
            error(errSyntaxWarning, -1, "Bad Pattern color space (underlying color space)");
            return {};
        }
    }
    return std::make_unique<GfxPatternColorSpace>(std::move(underA));
}

void GfxPatternColorSpace::getGray(const GfxColor * /*color*/, GfxGray *gray) const
{
    *gray = 0;
}