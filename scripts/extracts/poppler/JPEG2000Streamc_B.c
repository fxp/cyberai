// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPEG2000Stream.cc
// Segment 2/3



    if (unlikely(jpxData->size <= jpxData->pos)) {
        return (OPJ_SIZE_T)-1; /* End of file! */
    }
    OPJ_SIZE_T len = jpxData->size - jpxData->pos;
    if (len > p_nb_bytes) {
        len = p_nb_bytes;
    }
    memcpy(p_buffer, jpxData->data + jpxData->pos, len);
    jpxData->pos += len;
    return len;
}

static OPJ_OFF_T jpxSkip_callback(OPJ_OFF_T skip, void *p_user_data)
{
    auto *jpxData = (JPXData *)p_user_data;

    jpxData->pos += (skip > jpxData->size - jpxData->pos) ? jpxData->size - jpxData->pos : skip;
    /* Always return input value to avoid "Problem with skipping JPEG2000 box, stream error" */
    return skip;
}

static OPJ_BOOL jpxSeek_callback(OPJ_OFF_T seek_pos, void *p_user_data)
{
    auto *jpxData = (JPXData *)p_user_data;

    if (seek_pos > jpxData->size) {
        return OPJ_FALSE;
    }
    jpxData->pos = seek_pos;
    return OPJ_TRUE;
}

void JPXStream::init()
{
    Object oLen, cspace, smaskInDataObj;
    if (getDict()) {
        oLen = getDict()->lookup("Length");
        cspace = getDict()->lookup("ColorSpace");
        smaskInDataObj = getDict()->lookup("SMaskInData");
    }

    int bufSize = BUFFER_INITIAL_SIZE;
    if (oLen.isInt() && oLen.getInt() > 0) {
        bufSize = oLen.getInt();
    }

    bool indexed = false;
    if (cspace.isArrayOfLengthAtLeast(1)) {
        const Object cstype = cspace.arrayGet(0);
        if (cstype.isName("Indexed")) {
            indexed = true;
        }
    }

    const int smaskInData = smaskInDataObj.isInt() ? smaskInDataObj.getInt() : 0;
    const std::vector<unsigned char> buf = str->toUnsignedChars(bufSize);
    priv->init2(OPJ_CODEC_JP2, buf.data(), buf.size(), indexed);

    if (priv->image) {
        int numComps = priv->image->numcomps;
        int alpha = 0;
        if (priv->image->color_space == OPJ_CLRSPC_SRGB && numComps == 4) {
            numComps = 3;
            alpha = 1;
        } else if (priv->image->color_space == OPJ_CLRSPC_SYCC && numComps == 4) {
            numComps = 3;
            alpha = 1;
        } else if (numComps == 2) {
            numComps = 1;
            alpha = 1;
        } else if (numComps > 4) {
            numComps = 4;
            alpha = 1;
        } else {
            alpha = 0;
        }
        priv->npixels = priv->image->comps[0].w * priv->image->comps[0].h;
        priv->ncomps = priv->image->numcomps;
        if (alpha == 1 && smaskInData == 0 && !supportJPXtransparency()) {
            priv->ncomps--;
        }
        for (int component = 0; component < priv->ncomps; component++) {
            if (priv->image->comps[component].data == nullptr) {
                close();
                break;
            }
            const int componentPixels = priv->image->comps[component].w * priv->image->comps[component].h;
            if (componentPixels != priv->npixels) {
                error(errSyntaxWarning, -1, "Component {0:d} has different WxH than component 0", component);
                close();
                break;
            }
            auto *cdata = (unsigned char *)priv->image->comps[component].data;
            int adjust = 0;
            int depth = priv->image->comps[component].prec;
            if (priv->image->comps[component].prec > 8) {
                adjust = priv->image->comps[component].prec - 8;
            }
            int sgndcorr = 0;
            if (priv->image->comps[component].sgnd) {
                sgndcorr = 1 << (priv->image->comps[0].prec - 1);
            }
            for (int i = 0; i < priv->npixels; i++) {
                int r = priv->image->comps[component].data[i];
                *(cdata++) = adjustComp(r, adjust, depth, sgndcorr, indexed);
            }
        }
    } else {
        priv->npixels = 0;
    }

    priv->counter = 0;
    priv->ccounter = 0;
    priv->inited = true;
}

void JPXStreamPrivate::init2(OPJ_CODEC_FORMAT format, const unsigned char *buf, int length, bool indexed)
{
    JPXData jpxData;

    jpxData.data = buf;
    jpxData.pos = 0;
    jpxData.size = length;

    opj_stream_t *stream;

    stream = opj_stream_default_create(OPJ_TRUE);

    opj_stream_set_user_data(stream, &jpxData, nullptr);

    opj_stream_set_read_function(stream, jpxRead_callback);
    opj_stream_set_skip_function(stream, jpxSkip_callback);
    opj_stream_set_seek_function(stream, jpxSeek_callback);
    /* Set the length to avoid an assert */
    opj_stream_set_user_data_length(stream, length);

    opj_codec_t *decoder;

    /* Use default decompression parameters */
    opj_dparameters_t parameters;
    opj_set_default_decoder_parameters(&parameters);
    if (indexed) {
        parameters.flags |= OPJ_DPARAMETERS_IGNORE_PCLR_CMAP_CDEF_FLAG;
    }

    /* Get the decoder handle of the format */
    decoder = opj_create_decompress(format);
    if (decoder == nullptr) {
        error(errSyntaxWarning, -1, "Unable to create decoder");
        goto error;
    }