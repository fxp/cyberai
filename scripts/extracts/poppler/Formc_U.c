// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 21/24



        FT_Face face;
        if (ft_new_face_from_file(freetypeLib, filepath.c_str(), faceIndex, &face)) {
            error(errIO, -1, "ft_new_face_from_file failed for {0:s}", filepath.c_str());
            return {};
        }
        const std::unique_ptr<FT_Face, void (*)(FT_Face *)> faceDeleter(&face, [](FT_Face *f) { FT_Done_Face(*f); });

        if (FT_Set_Char_Size(face, 1000, 1000, 0, 0)) {
            error(errIO, -1, "FT_Set_Char_Size failed for {0:s}", filepath.c_str());
            return {};
        }

        {
            std::unique_ptr<Dict> fontDescriptor = std::make_unique<Dict>(xref);
            fontDescriptor->set("Type", Object(objName, "FontDescriptor"));
            fontDescriptor->set("FontName", Object(objName, fontFamilyAndStyle.c_str()));

            // a bit arbirary but the Flags field is mandatory...
            const std::string lowerCaseFontFamily = GooString::toLowerCase(fontFamily);
            if (lowerCaseFontFamily.contains("serif") && !lowerCaseFontFamily.contains("sans")) {
                fontDescriptor->set("Flags", Object(2)); // Serif
            } else {
                fontDescriptor->set("Flags", Object(0)); // Sans Serif
            }

            auto fontBBox = std::make_unique<Array>(xref);
            fontBBox->add(Object(static_cast<int>(face->bbox.xMin)));
            fontBBox->add(Object(static_cast<int>(face->bbox.yMin)));
            fontBBox->add(Object(static_cast<int>(face->bbox.xMax)));
            fontBBox->add(Object(static_cast<int>(face->bbox.yMax)));
            fontDescriptor->set("FontBBox", Object(std::move(fontBBox)));

            fontDescriptor->set("Ascent", Object(static_cast<int>(face->ascender)));

            fontDescriptor->set("Descent", Object(static_cast<int>(face->descender)));

            {
                const std::unique_ptr<GooFile> file(GooFile::open(filepath));
                if (!file) {
                    error(errIO, -1, "Failed to open {0:s}", filepath.c_str());
                    return {};
                }
                const Goffset fileSize = file->size();
                if (fileSize < 0) {
                    error(errIO, -1, "Failed to get file size for {0:s}", filepath.c_str());
                    return {};
                }
                // GooFile::read only takes an integer so for now we don't support huge fonts
                if (fileSize > std::numeric_limits<int>::max()) {
                    error(errIO, -1, "Font size is too big {0:s}", filepath.c_str());
                    return {};
                }
                std::vector<char> data;
                data.resize(fileSize);
                const Goffset bytesRead = file->read(data.data(), static_cast<int>(fileSize), 0);
                if (bytesRead != fileSize) {
                    error(errIO, -1, "Failed to read contents of {0:s}", filepath.c_str());
                    return {};
                }

                if (isTrueType) {
                    const Ref fontFile2Ref = xref->addStreamObject(std::make_unique<Dict>(xref), std::move(data), StreamCompression::Compress);
                    fontDescriptor->set("FontFile2", Object(fontFile2Ref));
                } else {
                    auto fontFileStreamDict = std::make_unique<Dict>(xref);
                    fontFileStreamDict->set("Subtype", Object(objName, "OpenType"));
                    const Ref fontFile3Ref = xref->addStreamObject(std::move(fontFileStreamDict), std::move(data), StreamCompression::Compress);
                    fontDescriptor->set("FontFile3", Object(fontFile3Ref));
                }
            }

            const Ref fontDescriptorRef = xref->addIndirectObject(Object(std::move(fontDescriptor)));
            descendantFont->set("FontDescriptor", Object(fontDescriptorRef));
        }

        static const int basicMultilingualMaxCode = 65535;

        const std::unique_ptr<FoFiTrueType> fft = FoFiTrueType::load(filepath.c_str(), faceIndex);
        if (fft) {

            // Look for the Unicode BMP cmaps, which are 0/3 or 3/1
            int unicodeBMPCMap = fft->findCmap(0, 3);
            if (unicodeBMPCMap < 0) {
                unicodeBMPCMap = fft->findCmap(3, 1);
            }
            if (unicodeBMPCMap < 0) {
                error(errIO, -1, "Font does not have an unicode BMP cmap {0:s}", filepath.c_str());
                return {};
            }

            CIDFontsWidthsBuilder fontsWidths;