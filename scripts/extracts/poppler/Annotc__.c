// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 31/79



#define ANNOT_TEXT_AP_UP_LEFT_ARROW                                                                                                                                                                                                            \
    "0.9 0.2 0.8 RG 2.5 w\n"                                                                                                                                                                                                                   \
    "1 J\n"                                                                                                                                                                                                                                    \
    "1 j\n"                                                                                                                                                                                                                                    \
    "[] 0.0 d\n"                                                                                                                                                                                                                               \
    "4 M 18 6 m 6 18 l S\n"                                                                                                                                                                                                                    \
    "6 18 m 11 18 l S\n"                                                                                                                                                                                                                       \
    "6 18 m 6 13 l S\n"

#define ANNOT_TEXT_AP_CROSS_HAIRS                                                                                                                                                                                                              \
    "0.2 0.8 0.3 RG 2.5 w\n"                                                                                                                                                                                                                   \
    "1 J\n"                                                                                                                                                                                                                                    \
    "1 j\n"                                                                                                                                                                                                                                    \
    "[] 0.0 d\n"                                                                                                                                                                                                                               \
    "4 M 12 10 m 12 14 l S\n"                                                                                                                                                                                                                  \
    "10 12 m 14 12 l S\n"                                                                                                                                                                                                                      \
    "17 12 m 17 9.239 14.761 7 12 7 c 9.239 7 7 9.239 7 12 c 7 14.761 9.239 17 12 17 c 14.761 17 17 14.761 17 12 c h S\n"

void AnnotText::draw(Gfx *gfx, bool printing)
{
    double ca = 1;

    if (!isVisible(printing)) {
        return;
    }

    annotLocker();
    if (appearance.isNull()) {
        ca = opacity;

        AnnotAppearanceBuilder appearBuilder;