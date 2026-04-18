// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxState.cc
// Segment 20/49



GfxDeviceNColorSpace::GfxDeviceNColorSpace(int nCompsA, std::vector<std::string> &&namesA, std::unique_ptr<GfxColorSpace> &&altA, std::unique_ptr<Function> funcA, std::vector<std::unique_ptr<GfxSeparationColorSpace>> &&sepsCSA)
    : nComps(nCompsA), names(std::move(namesA)), alt(std::move(altA))
{
    func = std::move(funcA);
    sepsCS = std::move(sepsCSA);
    nonMarking = true;
    overprintMask = 0;
    for (int i = 0; i < nComps; ++i) {
        if (names[i] != "None") {
            nonMarking = false;
        }
        if (names[i] == "Cyan") {
            overprintMask |= 0x01;
        } else if (names[i] == "Magenta") {
            overprintMask |= 0x02;
        } else if (names[i] == "Yellow") {
            overprintMask |= 0x04;
        } else if (names[i] == "Black") {
            overprintMask |= 0x08;
        } else if (names[i] == "All") {
            overprintMask = 0xffffffff;
        } else if (names[i] != "None") {
            overprintMask = 0x0f;
        }
    }
}

GfxDeviceNColorSpace::GfxDeviceNColorSpace(int nCompsA, const std::vector<std::string> &namesA, std::unique_ptr<GfxColorSpace> &&altA, std::unique_ptr<Function> funcA, std::vector<std::unique_ptr<GfxSeparationColorSpace>> &&sepsCSA,
                                           const std::vector<int> &mappingA, bool nonMarkingA, unsigned int overprintMaskA, PrivateTag /*unused*/)
    : nComps(nCompsA), names(namesA), alt(std::move(altA))
{
    func = std::move(funcA);
    sepsCS = std::move(sepsCSA);
    mapping = mappingA;
    nonMarking = nonMarkingA;
    overprintMask = overprintMaskA;
}

GfxDeviceNColorSpace::~GfxDeviceNColorSpace() = default;

std::unique_ptr<GfxColorSpace> GfxDeviceNColorSpace::copy() const
{
    std::vector<std::unique_ptr<GfxSeparationColorSpace>> sepsCSA;
    sepsCSA.reserve(sepsCS.size());
    for (const std::unique_ptr<GfxSeparationColorSpace> &scs : sepsCS) {
        if (likely(scs != nullptr)) {
            sepsCSA.push_back(scs->copyAsOwnType());
        }
    }
    return std::make_unique<GfxDeviceNColorSpace>(nComps, names, alt->copy(), func->copy(), std::move(sepsCSA), mapping, nonMarking, overprintMask);
}

//~ handle the 'None' colorant
std::unique_ptr<GfxColorSpace> GfxDeviceNColorSpace::parse(GfxResources *res, const Array &arr, OutputDev *out, GfxState *state, int recursion)
{
    int nCompsA;
    std::vector<std::string> namesA;
    std::unique_ptr<GfxColorSpace> altA;
    std::unique_ptr<Function> funcA;
    Object obj1;
    std::vector<std::unique_ptr<GfxSeparationColorSpace>> separationList;

    if (arr.getLength() != 4 && arr.getLength() != 5) {
        error(errSyntaxWarning, -1, "Bad DeviceN color space");
        return nullptr;
    }
    obj1 = arr.get(1);
    if (!obj1.isArray()) {
        error(errSyntaxWarning, -1, "Bad DeviceN color space (names)");
        return nullptr;
    }
    nCompsA = obj1.arrayGetLength();
    if (nCompsA > gfxColorMaxComps) {
        error(errSyntaxWarning, -1, "DeviceN color space with too many ({0:d} > {1:d}) components", nCompsA, gfxColorMaxComps);
        nCompsA = gfxColorMaxComps;
    }
    for (int i = 0; i < nCompsA; ++i) {
        Object obj2 = obj1.arrayGet(i);
        if (!obj2.isName()) {
            error(errSyntaxWarning, -1, "Bad DeviceN color space (names)");
            nCompsA = i;
            return nullptr;
        }
        namesA.emplace_back(obj2.getName());
    }
    obj1 = arr.get(2);
    if (!(altA = GfxColorSpace::parse(res, &obj1, out, state, recursion + 1))) {
        error(errSyntaxWarning, -1, "Bad DeviceN color space (alternate color space)");
        return nullptr;
    }
    obj1 = arr.get(3);
    if (!(funcA = Function::parse(&obj1))) {
        return nullptr;
    }
    if (arr.getLength() == 5) {
        obj1 = arr.get(4);
        if (!obj1.isDict()) {
            error(errSyntaxWarning, -1, "Bad DeviceN color space (attributes)");
            return nullptr;
        }
        Dict *attribs = obj1.getDict();
        Object obj2 = attribs->lookup("Colorants");
        if (obj2.isDict()) {
            Dict *colorants = obj2.getDict();
            for (int i = 0; i < colorants->getLength(); i++) {
                Object obj3 = colorants->getVal(i);
                if (obj3.isArray()) {
                    auto cs = GfxSeparationColorSpace::parse(res, *obj3.getArray(), out, state, recursion);
                    if (cs) {
                        separationList.push_back(std::unique_ptr<GfxSeparationColorSpace>(static_cast<GfxSeparationColorSpace *>(cs.release())));
                    }
                } else {
                    error(errSyntaxWarning, -1, "Bad DeviceN color space (colorant value entry is not an Array)");
                    return nullptr;
                }
            }
        }
    }