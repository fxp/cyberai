// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPEG2000Stream.cc
// Segment 1/3

//========================================================================
//
// JPEG2000Stream.cc
//
// A JPX stream decoder using OpenJPEG
//
// Copyright 2008-2010, 2012, 2017-2023, 2025, 2026 Albert Astals Cid <aacid@kde.org>
// Copyright 2011 Daniel Glöckner <daniel-gl@gmx.net>
// Copyright 2014, 2016 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright 2013, 2014 Adrian Johnson <ajohnson@redneon.com>
// Copyright 2015 Adam Reichold <adam.reichold@t-online.de>
// Copyright 2015 Jakub Wilk <jwilk@jwilk.net>
// Copyright 2022 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright 2024, 2025 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
// Copyright (C) 2026 Adam Sampson <ats@offog.org>
//
// Licensed under GPLv2 or later
//
//========================================================================

#include "config.h"
#include "JPEG2000Stream.h"
#include <openjpeg.h>

struct JPXStreamPrivate
{
    opj_image_t *image = nullptr;
    int counter = 0;
    int ccounter = 0;
    int npixels = 0;
    int ncomps = 0;
    bool inited = false;
    void init2(OPJ_CODEC_FORMAT format, const unsigned char *buf, int length, bool indexed);
};

static inline unsigned char adjustComp(int r, int adjust, int depth, int sgndcorr, bool indexed)
{
    if (!indexed) {
        r += sgndcorr;
        if (adjust) {
            r = (r >> adjust) + ((r >> (adjust - 1)) % 2);
        } else if (depth < 8) {
            r = r << (8 - depth);
        }
    }
    if (unlikely(r > 255)) {
        r = 255;
    }
    return r;
}

static inline int doLookChar(JPXStreamPrivate *priv)
{
    if (unlikely(priv->counter >= priv->npixels)) {
        return EOF;
    }

    return ((unsigned char *)priv->image->comps[priv->ccounter].data)[priv->counter];
}

static inline int doGetChar(JPXStreamPrivate *priv)
{
    const int result = doLookChar(priv);
    if (++priv->ccounter == priv->ncomps) {
        priv->ccounter = 0;
        ++priv->counter;
    }
    return result;
}

JPXStream::JPXStream(std::unique_ptr<Stream> strA) : OwnedFilterStream(std::move(strA))
{
    priv = new JPXStreamPrivate;
    handleJPXtransparency = false;
}

JPXStream::~JPXStream()
{
    close();
    delete priv;
}

bool JPXStream::rewind()
{
    priv->counter = 0;
    priv->ccounter = 0;

    return true;
}

void JPXStream::close()
{
    if (priv->image != nullptr) {
        opj_image_destroy(priv->image);
        priv->image = nullptr;
        priv->npixels = 0;
    }
}

Goffset JPXStream::getPos()
{
    return priv->counter * priv->ncomps + priv->ccounter;
}

int JPXStream::getChars(int nChars, unsigned char *buffer)
{
    if (unlikely(priv->inited == false)) {
        init();
    }

    for (int i = 0; i < nChars; ++i) {
        const int c = doGetChar(priv);
        if (likely(c != EOF)) {
            buffer[i] = c;
        } else {
            return i;
        }
    }
    return nChars;
}

int JPXStream::getChar()
{
    if (unlikely(priv->inited == false)) {
        init();
    }

    return doGetChar(priv);
}

int JPXStream::lookChar()
{
    if (unlikely(priv->inited == false)) {
        init();
    }

    return doLookChar(priv);
}

std::optional<std::string> JPXStream::getPSFilter(int /*psLevel*/, const char * /*indent*/)
{
    return {};
}

bool JPXStream::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}

void JPXStream::getImageParams(int *bitsPerComponent, StreamColorSpaceMode *csMode, bool *hasAlpha)
{
    if (unlikely(priv->inited == false)) {
        init();
    }

    *bitsPerComponent = 8;
    *hasAlpha = false;
    int numComps = (priv->image) ? priv->image->numcomps : 1;
    if (priv->image) {
        if (priv->image->color_space == OPJ_CLRSPC_SRGB && numComps == 4) {
            numComps = 3;
            *hasAlpha = true;
        } else if (priv->image->color_space == OPJ_CLRSPC_SYCC && numComps == 4) {
            numComps = 3;
            *hasAlpha = true;
        } else if (numComps == 2) {
            numComps = 1;
        } else if (numComps > 4) {
            *hasAlpha = true;
            numComps = 4;
        }
    }
    if (numComps == 3) {
        *csMode = streamCSDeviceRGB;
    } else if (numComps == 4) {
        *csMode = streamCSDeviceCMYK;
    } else {
        *csMode = streamCSDeviceGray;
    }
}

static void libopenjpeg_error_callback(const char *msg, void * /*client_data*/)
{
    error(errSyntaxError, -1, "{0:s}", msg);
}

static void libopenjpeg_warning_callback(const char *msg, void * /*client_data*/)
{
    error(errSyntaxWarning, -1, "{0:s}", msg);
}

struct JPXData
{
    const unsigned char *data;
    int size;
    OPJ_OFF_T pos;
};

constexpr int BUFFER_INITIAL_SIZE = 4096;

static OPJ_SIZE_T jpxRead_callback(void *p_buffer, OPJ_SIZE_T p_nb_bytes, void *p_user_data)
{
    auto *jpxData = (JPXData *)p_user_data;