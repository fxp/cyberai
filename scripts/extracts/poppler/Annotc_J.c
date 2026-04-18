// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Annot.cc
// Segment 42/79



                appearBuilder.appendf("{0:.2f} {1:.2f} m\n", x3, y3);
                appearBuilder.appendf("{0:.2f} {1:.2f} l\n", x4, y4);
                appearBuilder.append("S\n");
            }
            break;
        case typeStrikeOut:
            if (color) {
                appearBuilder.setDrawColor(*color, false);
            }
            blendMultiply = false;
            appearBuilder.append("[] 0 d 1 w\n");

            for (i = 0; i < quadrilaterals->getQuadrilateralsLength(); ++i) {
                double x1, y1, x2, y2;
                double x3, y3, x4, y4;

                x1 = quadrilaterals->getX1(i);
                y1 = quadrilaterals->getY1(i);
                x2 = quadrilaterals->getX2(i);
                y2 = quadrilaterals->getY2(i);

                x3 = quadrilaterals->getX3(i);
                y3 = quadrilaterals->getY3(i);
                x4 = quadrilaterals->getX4(i);
                y4 = quadrilaterals->getY4(i);

                appearBuilder.appendf("{0:.2f} {1:.2f} m\n", (x1 + x3) / 2., (y1 + y3) / 2.);
                appearBuilder.appendf("{0:.2f} {1:.2f} l\n", (x2 + x4) / 2., (y2 + y4) / 2.);
                appearBuilder.append("S\n");
            }
            break;
        case typeSquiggly:
            if (color) {
                appearBuilder.setDrawColor(*color, false);
            }
            appearBuilder.append("[] 0 d 1 w\n");

            for (i = 0; i < quadrilaterals->getQuadrilateralsLength(); ++i) {
                double x1, y1, x2, y3;
                double h6;

                x1 = quadrilaterals->getX1(i);
                y1 = quadrilaterals->getY1(i);
                x2 = quadrilaterals->getX2(i);
                y3 = quadrilaterals->getY3(i);
                h6 = (y1 - y3) / 6.0;

                appearBuilder.appendf("{0:.2f} {1:.2f} m\n", x1, y3 + h6);
                bool down = false;
                do {
                    down = !down; // Zigzag line
                    x1 += 2;
                    appearBuilder.appendf("{0:.2f} {1:.2f} l\n", x1, y3 + (down ? 0 : h6));
                } while (x1 < x2);
                appearBuilder.append("S\n");
            }
            break;
        default:
        case typeHighlight:
            if (color) {
                appearBuilder.setDrawColor(*color, true);
            }

            double biggestBorder = 0;
            for (i = 0; i < quadrilaterals->getQuadrilateralsLength(); ++i) {
                double x1, y1, x2, y2, x3, y3, x4, y4;
                double h4;

                x1 = quadrilaterals->getX1(i);
                y1 = quadrilaterals->getY1(i);
                x2 = quadrilaterals->getX2(i);
                y2 = quadrilaterals->getY2(i);
                x3 = quadrilaterals->getX3(i);
                y3 = quadrilaterals->getY3(i);
                x4 = quadrilaterals->getX4(i);
                y4 = quadrilaterals->getY4(i);
                h4 = fabs(y1 - y3) / 4.0;

                if (h4 > biggestBorder) {
                    biggestBorder = h4;
                }

                appearBuilder.appendf("{0:.2f} {1:.2f} m\n", x3, y3);
                appearBuilder.appendf("{0:.2f} {1:.2f} {2:.2f} {3:.2f} {4:.2f} {5:.2f} c\n", x3 - h4, y3 + h4, x1 - h4, y1 - h4, x1, y1);
                appearBuilder.appendf("{0:.2f} {1:.2f} l\n", x2, y2);
                appearBuilder.appendf("{0:.2f} {1:.2f} {2:.2f} {3:.2f} {4:.2f} {5:.2f} c\n", x2 + h4, y2 - h4, x4 + h4, y4 + h4, x4, y4);
                appearBuilder.append("f\n");
            }
            appearBBox->setBorderWidth(biggestBorder);
            break;
        }
        appearBuilder.append("Q\n");

        const std::array<double, 4> bbox = { appearBBox->getPageXMin(), appearBBox->getPageYMin(), appearBBox->getPageXMax(), appearBBox->getPageYMax() };
        Object aStream = createForm(appearBuilder.buffer(), bbox, true, Object {});

        GooString appearBuf("/GS0 gs\n/Fm0 Do");
        std::unique_ptr<Dict> resDict = createResourcesDict("Fm0", std::move(aStream), "GS0", 1, blendMultiply ? "Multiply" : nullptr);
        if (ca == 1) {
            appearance = createForm(&appearBuf, bbox, false, std::move(resDict));
        } else {
            aStream = createForm(&appearBuf, bbox, true, std::move(resDict));

            std::unique_ptr<Dict> resDict2 = createResourcesDict("Fm0", std::move(aStream), "GS0", ca, nullptr);
            appearance = createForm(&appearBuf, bbox, false, std::move(resDict2));
        }
    }

    // draw the appearance stream
    Object obj = appearance.fetch(gfx->getXRef());
    if (appearBBox) {
        gfx->drawAnnot(&obj, nullptr, color.get(), appearBBox->getPageXMin(), appearBBox->getPageYMin(), appearBBox->getPageXMax(), appearBBox->getPageYMax(), getRotation());
    } else {
        gfx->drawAnnot(&obj, nullptr, color.get(), rect->x1, rect->y1, rect->x2, rect->y2, getRotation());
    }
}