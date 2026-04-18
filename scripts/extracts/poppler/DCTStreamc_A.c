// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/DCTStream.cc
// Segment 1/2

//========================================================================
//
// DCTStream.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2005 Jeff Muizelaar <jeff@infidigm.net>
// Copyright 2005-2010, 2012, 2017, 2020-2023, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright 2009 Ryszard Trojnacki <rysiek@menel.com>
// Copyright 2010 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright 2011 Daiki Ueno <ueno@unixuser.org>
// Copyright 2011 Tomas Hoger <thoger@redhat.com>
// Copyright 2012, 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright 2020 Lluís Batlle i Rossell <viric@viric.name>
// Copyright 2025 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
//
//========================================================================

#include "DCTStream.h"

static void str_init_source(j_decompress_ptr /*cinfo*/) { }

static boolean str_fill_input_buffer(j_decompress_ptr cinfo)
{
    int c;
    auto *src = (struct str_src_mgr *)cinfo->src;
    if (src->index == 0) {
        c = 0xFF;
        src->index++;
    } else if (src->index == 1) {
        c = 0xD8;
        src->index++;
    } else {
        c = src->str->getChar();
    }
    src->buffer = c;
    src->pub.next_input_byte = &src->buffer;
    src->pub.bytes_in_buffer = 1;
    if (c != EOF) {
        return TRUE;
    }
    return FALSE;
}

static void str_skip_input_data(j_decompress_ptr cinfo, long num_bytes_l)
{
    if (num_bytes_l <= 0) {
        return;
    }

    size_t num_bytes = num_bytes_l;

    auto *src = (struct str_src_mgr *)cinfo->src;
    while (num_bytes > src->pub.bytes_in_buffer) {
        num_bytes -= src->pub.bytes_in_buffer;
        str_fill_input_buffer(cinfo);
    }
    src->pub.next_input_byte += num_bytes;
    src->pub.bytes_in_buffer -= num_bytes;
}

static void str_term_source(j_decompress_ptr /*cinfo*/) { }

DCTStream::DCTStream(std::unique_ptr<Stream> strA, int colorXformA, Dict *dict, int recursion) : OwnedFilterStream(std::move(strA))
{
    colorXform = colorXformA;
    if (dict != nullptr) {
        Object obj = dict->lookup("Width", recursion);
        err.width = (obj.isInt() && obj.getInt() <= JPEG_MAX_DIMENSION) ? obj.getInt() : 0;
        obj = dict->lookup("Height", recursion);
        err.height = (obj.isInt() && obj.getInt() <= JPEG_MAX_DIMENSION) ? obj.getInt() : 0;
    } else {
        err.height = err.width = 0;
    }
    init();
}

DCTStream::~DCTStream()
{
    jpeg_destroy_decompress(&cinfo);
}

static void exitErrorHandler(jpeg_common_struct *error)
{
    auto *cinfo = (j_decompress_ptr)error;
    auto *err = (struct str_error_mgr *)cinfo->err;
    if (cinfo->err->msg_code == JERR_IMAGE_TOO_BIG && err->width != 0 && err->height != 0) {
        cinfo->image_height = err->height;
        cinfo->image_width = err->width;
    } else {
        longjmp(err->setjmp_buffer, 1);
    }
}

void DCTStream::init()
{
    jpeg_std_error(&err.pub);
    err.pub.error_exit = &exitErrorHandler;
    src.pub.init_source = str_init_source;
    src.pub.fill_input_buffer = str_fill_input_buffer;
    src.pub.skip_input_data = str_skip_input_data;
    src.pub.resync_to_restart = jpeg_resync_to_restart;
    src.pub.term_source = str_term_source;
    src.pub.bytes_in_buffer = 0;
    src.pub.next_input_byte = nullptr;
    src.str = str;
    src.index = 0;
    current = nullptr;
    limit = nullptr;

    cinfo.err = &err.pub;
    if (!setjmp(err.setjmp_buffer)) {
        jpeg_create_decompress(&cinfo);
        cinfo.src = (jpeg_source_mgr *)&src;
    }
    row_buffer = nullptr;
}

bool DCTStream::rewind()
{
    int row_stride;

    bool success = str->rewind();

    if (row_buffer) {
        jpeg_destroy_decompress(&cinfo);
        init();
    }

    // JPEG data has to start with 0xFF 0xD8
    // but some pdf like the one on
    // https://bugs.freedesktop.org/show_bug.cgi?id=3299
    // does have some garbage before that this seeks for
    // the start marker...
    bool startFound = false;
    int c = 0, c2 = 0;
    while (!startFound) {
        if (!c) {
            c = str->getChar();
            if (c == -1) {
                error(errSyntaxError, -1, "Could not find start of jpeg data");
                return false;
            }
            if (c != 0xFF) {
                c = 0;
            }
        } else {
            c2 = str->getChar();
            if (c2 != 0xD8) {
                c = 0;
                c2 = 0;
            } else {
                startFound = true;
            }
        }
    }