// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/DCTStream.cc
// Segment 2/2



    if (!setjmp(err.setjmp_buffer)) {
        if (jpeg_read_header(&cinfo, TRUE) != JPEG_SUSPENDED) {
            // figure out color transform
            if (colorXform == -1 && !cinfo.saw_Adobe_marker) {
                if (cinfo.num_components == 3) {
                    if (cinfo.saw_JFIF_marker) {
                        colorXform = 1;
                    } else if (cinfo.cur_comp_info[0] && cinfo.cur_comp_info[1] && cinfo.cur_comp_info[2] && cinfo.cur_comp_info[0]->component_id == 82 && cinfo.cur_comp_info[1]->component_id == 71
                               && cinfo.cur_comp_info[2]->component_id == 66) { // ASCII "RGB"
                        colorXform = 0;
                    } else {
                        colorXform = 1;
                    }
                } else {
                    colorXform = 0;
                }
            } else if (cinfo.saw_Adobe_marker) {
                colorXform = cinfo.Adobe_transform;
            }

            switch (cinfo.num_components) {
            case 3:
                cinfo.jpeg_color_space = colorXform ? JCS_YCbCr : JCS_RGB;
                break;
            case 4:
                cinfo.jpeg_color_space = colorXform ? JCS_YCCK : JCS_CMYK;
                break;
            }

            jpeg_start_decompress(&cinfo);

            row_stride = cinfo.output_width * cinfo.output_components;
            row_buffer = cinfo.mem->alloc_sarray((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
        }
    }

    return success;
}

bool DCTStream::readLine()
{
    if (cinfo.output_scanline < cinfo.output_height) {
        if (!setjmp(err.setjmp_buffer)) {
            if (!jpeg_read_scanlines(&cinfo, row_buffer, 1)) {
                return false;
            }
            current = &row_buffer[0][0];
            limit = &row_buffer[0][(cinfo.output_width - 1) * cinfo.output_components] + cinfo.output_components;
            return true;
        }
        return false;
    }
    return false;
}

int DCTStream::getChar()
{
    if (current == limit) {
        if (!readLine()) {
            return EOF;
        }
    }

    return *current++;
}

int DCTStream::getChars(int nChars, unsigned char *buffer)
{
    for (int i = 0; i < nChars;) {
        if (current == limit) {
            if (!readLine()) {
                return i;
            }
        }
        intptr_t left = limit - current;
        if (left + i > nChars) {
            left = nChars - i;
        }
        memcpy(buffer + i, current, left);
        current += left;
        i += static_cast<int>(left);
    }
    return nChars;
}

int DCTStream::lookChar()
{
    if (unlikely(current == nullptr)) {
        return EOF;
    }
    return *current;
}

std::optional<std::string> DCTStream::getPSFilter(int psLevel, const char *indent)
{
    std::optional<std::string> s;

    if (psLevel < 2) {
        return {};
    }
    if (!(s = str->getPSFilter(psLevel, indent))) {
        return {};
    }
    s->append(indent).append("<< >> /DCTDecode filter\n");
    return s;
}

bool DCTStream::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}
