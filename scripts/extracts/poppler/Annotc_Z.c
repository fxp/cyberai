// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 58/79



    double stampUnscaledWidth;
    double stampUnscaledHeight;
    const char *stampCode;
    if (icon == "Approved") {
        stampUnscaledWidth = ANNOT_STAMP_APPROVED_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_APPROVED_HEIGHT;
        stampCode = ANNOT_STAMP_APPROVED;
        extGStateDict = getApprovedStampExtGStateDict(doc);
    } else if (icon == "AsIs") {
        stampUnscaledWidth = ANNOT_STAMP_AS_IS_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_AS_IS_HEIGHT;
        stampCode = ANNOT_STAMP_AS_IS;
        extGStateDict = getAsIsStampExtGStateDict(doc);
    } else if (icon == "Confidential") {
        stampUnscaledWidth = ANNOT_STAMP_CONFIDENTIAL_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_CONFIDENTIAL_HEIGHT;
        stampCode = ANNOT_STAMP_CONFIDENTIAL;
        extGStateDict = getConfidentialStampExtGStateDict(doc);
    } else if (icon == "Final") {
        stampUnscaledWidth = ANNOT_STAMP_FINAL_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_FINAL_HEIGHT;
        stampCode = ANNOT_STAMP_FINAL;
        extGStateDict = getFinalStampExtGStateDict(doc);
    } else if (icon == "Experimental") {
        stampUnscaledWidth = ANNOT_STAMP_EXPERIMENTAL_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_EXPERIMENTAL_HEIGHT;
        stampCode = ANNOT_STAMP_EXPERIMENTAL;
        extGStateDict = getExperimentalStampExtGStateDict(doc);
    } else if (icon == "Expired") {
        stampUnscaledWidth = ANNOT_STAMP_EXPIRED_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_EXPIRED_HEIGHT;
        stampCode = ANNOT_STAMP_EXPIRED;
        extGStateDict = getExpiredStampExtGStateDict(doc);
    } else if (icon == "NotApproved") {
        stampUnscaledWidth = ANNOT_STAMP_NOT_APPROVED_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_NOT_APPROVED_HEIGHT;
        stampCode = ANNOT_STAMP_NOT_APPROVED;
        extGStateDict = getNotApprovedStampExtGStateDict(doc);
    } else if (icon == "NotForPublicRelease") {
        stampUnscaledWidth = ANNOT_STAMP_NOT_FOR_PUBLIC_RELEASE_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_NOT_FOR_PUBLIC_RELEASE_HEIGHT;
        stampCode = ANNOT_STAMP_NOT_FOR_PUBLIC_RELEASE;
        extGStateDict = getNotForPublicReleaseStampExtGStateDict(doc);
    } else if (icon == "Sold") {
        stampUnscaledWidth = ANNOT_STAMP_SOLD_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_SOLD_HEIGHT;
        stampCode = ANNOT_STAMP_SOLD;
        extGStateDict = getSoldStampExtGStateDict(doc);
    } else if (icon == "Departmental") {
        stampUnscaledWidth = ANNOT_STAMP_DEPARTMENTAL_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_DEPARTMENTAL_HEIGHT;
        stampCode = ANNOT_STAMP_DEPARTMENTAL;
        extGStateDict = getDepartmentalStampExtGStateDict(doc);
    } else if (icon == "ForComment") {
        stampUnscaledWidth = ANNOT_STAMP_FOR_COMMENT_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_FOR_COMMENT_HEIGHT;
        stampCode = ANNOT_STAMP_FOR_COMMENT;
        extGStateDict = getForCommentStampExtGStateDict(doc);
    } else if (icon == "ForPublicRelease") {
        stampUnscaledWidth = ANNOT_STAMP_FOR_PUBLIC_RELEASE_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_FOR_PUBLIC_RELEASE_HEIGHT;
        stampCode = ANNOT_STAMP_FOR_PUBLIC_RELEASE;
        extGStateDict = getForPublicReleaseStampExtGStateDict(doc);
    } else if (icon == "TopSecret") {
        stampUnscaledWidth = ANNOT_STAMP_TOP_SECRET_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_TOP_SECRET_HEIGHT;
        stampCode = ANNOT_STAMP_TOP_SECRET;
        extGStateDict = getTopSecretStampExtGStateDict(doc);
    } else {
        stampUnscaledWidth = ANNOT_STAMP_DRAFT_WIDTH;
        stampUnscaledHeight = ANNOT_STAMP_DRAFT_HEIGHT;
        stampCode = ANNOT_STAMP_DRAFT;
        extGStateDict = getDraftStampExtGStateDict(doc);
    }

    const std::array<double, 4> bboxArray = { 0, 0, rect->x2 - rect->x1, rect->y2 - rect->y1 };
    const std::string scale = GooString::format("{0:.6g} 0 0 {1:.6g} 0 0 cm\nq\n", bboxArray[2] / stampUnscaledWidth, bboxArray[3] / stampUnscaledHeight);
    defaultAppearanceBuilder.append(scale.c_str());
    defaultAppearanceBuilder.append(stampCode);
    defaultAppearanceBuilder.append("Q\n");

    auto resDict = std::make_unique<Dict>(doc->getXRef());
    resDict->add("ExtGState", Object(std::move(extGStateDict)));

    Object aStream = createForm(defaultAppearanceBuilder.buffer(), bboxArray, true, std::move(resDict));

    AnnotAppearanceBuilder appearanceBuilder;
    appearanceBuilder.append("/GS0 gs\n/Fm0 Do");
    resDict = createResourcesDict("Fm0", std::move(aStream), "GS0", opacity, nullptr);
    appearance = createForm(appearanceBuilder.buffer(), bboxArray, false, std::move(resDict));
}

void AnnotStamp::draw(Gfx *gfx, bool printing)
{
    if (!isVisible(printing)) {
        return;
    }

    annotLocker();

    // generate the appearance stream
    updateAppearanceResDict();