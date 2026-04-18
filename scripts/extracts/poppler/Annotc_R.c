// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 50/79



        // write the font matrix (if not part of the DA string)
        if (tmPos < 0) {
            appearBuf->appendf("1 0 0 1 {0:.2f} {1:.2f} Tm\n", x, y);
        }

        // change the text color if selected
        if (fieldChoice->isSelected(i)) {
            appearBuf->append("1 g\n");
        }

        // write the text string
        writeString(convertedText.toStr());
        appearBuf->append(" Tj\n");

        // cleanup
        appearBuf->append("ET\n");
        appearBuf->append("Q\n");

        // next line
        y -= 1.1 * fontSize;
    }

    return true;
}

void AnnotAppearanceBuilder::drawFieldBorder(const FormField *field, const AnnotBorder *border, const AnnotAppearanceCharacs *appearCharacs, const PDFRectangle *rect)
{
    AnnotColor adjustedColor;
    const double w = border->getWidth();

    const AnnotColor *aColor = appearCharacs->getBorderColor();
    if (!aColor) {
        aColor = appearCharacs->getBackColor();
    }
    if (!aColor) {
        return;
    }

    const double dx = rect->x2 - rect->x1;
    const double dy = rect->y2 - rect->y1;

    // radio buttons with no caption have a round border
    const bool hasCaption = appearCharacs->getNormalCaption() != nullptr;
    if (field->getType() == formButton && static_cast<const FormFieldButton *>(field)->getButtonType() == formButtonRadio && !hasCaption) {
        double r = 0.5 * (dx < dy ? dx : dy);
        switch (border->getStyle()) {
        case AnnotBorder::borderDashed:
            appearBuf->append("[");
            for (double dash : border->getDash()) {
                appearBuf->appendf(" {0:.2f}", dash);
            }
            appearBuf->append("] 0 d\n");
            // fallthrough
        case AnnotBorder::borderSolid:
        case AnnotBorder::borderUnderlined:
            appearBuf->appendf("{0:.2f} w\n", w);
            setDrawColor(*aColor, false);
            drawCircle(0.5 * dx, 0.5 * dy, r - 0.5 * w, false);
            break;
        case AnnotBorder::borderBeveled:
        case AnnotBorder::borderInset:
            appearBuf->appendf("{0:.2f} w\n", 0.5 * w);
            setDrawColor(*aColor, false);
            drawCircle(0.5 * dx, 0.5 * dy, r - 0.25 * w, false);
            adjustedColor = AnnotColor(*aColor);
            adjustedColor.adjustColor(border->getStyle() == AnnotBorder::borderBeveled ? 1 : -1);
            setDrawColor(adjustedColor, false);
            drawCircleTopLeft(0.5 * dx, 0.5 * dy, r - 0.75 * w);
            adjustedColor = AnnotColor(*aColor);
            adjustedColor.adjustColor(border->getStyle() == AnnotBorder::borderBeveled ? -1 : 1);
            setDrawColor(adjustedColor, false);
            drawCircleBottomRight(0.5 * dx, 0.5 * dy, r - 0.75 * w);
            break;
        }
    } else {
        switch (border->getStyle()) {
        case AnnotBorder::borderDashed:
            appearBuf->append("[");
            for (double dash : border->getDash()) {
                appearBuf->appendf(" {0:.2f}", dash);
            }
            appearBuf->append("] 0 d\n");
            // fallthrough
        case AnnotBorder::borderSolid:
            appearBuf->appendf("{0:.2f} w\n", w);
            setDrawColor(*aColor, false);
            appearBuf->appendf("{0:.2f} {0:.2f} {1:.2f} {2:.2f} re s\n", 0.5 * w, dx - w, dy - w);
            break;
        case AnnotBorder::borderBeveled:
        case AnnotBorder::borderInset:
            adjustedColor = AnnotColor(*aColor);
            adjustedColor.adjustColor(border->getStyle() == AnnotBorder::borderBeveled ? 1 : -1);
            setDrawColor(adjustedColor, true);
            appearBuf->append("0 0 m\n");
            appearBuf->appendf("0 {0:.2f} l\n", dy);
            appearBuf->appendf("{0:.2f} {1:.2f} l\n", dx, dy);
            appearBuf->appendf("{0:.2f} {1:.2f} l\n", dx - w, dy - w);
            appearBuf->appendf("{0:.2f} {1:.2f} l\n", w, dy - w);
            appearBuf->appendf("{0:.2f} {0:.2f} l\n", w);
            appearBuf->append("f\n");
            adjustedColor = AnnotColor(*aColor);
            adjustedColor.adjustColor(border->getStyle() == AnnotBorder::borderBeveled ? -1 : 1);
            setDrawColor(adjustedColor, true);
            appearBuf->append("0 0 m\n");
            appearBuf->appendf("{0:.2f} 0 l\n", dx);
            appearBuf->appendf("{0:.2f} {1:.2f} l\n", dx, dy);
            appearBuf->appendf("{0:.2f} {1:.2f} l\n", dx - w, dy - w);
            appearBuf->appendf("{0:.2f} {1:.2f} l\n", dx - w, w);
            appearBuf->appendf("{0:.2f} {0:.2f} l\n", w);
            appearBuf->append("f\n");
            break;
        case AnnotBorder::borderUnderlined:
            appearBuf->appendf("{0:.2f} w\n", w);
            setDrawColor(*aColor, false);
            appearBuf->appendf("0 0 m {0:.2f} 0 l s\n", dx);
            break;
        }

        // clip to the inside of the border
        appearBuf->appendf("{0:.2f} {0:.2f} {1:.2f} {2:.2f} re W n\n", w, dx - 2 * w, dy - 2 * w);
    }
}