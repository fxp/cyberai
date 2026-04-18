// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Parser.cc
// Segment 1/3

//========================================================================
//
// Parser.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2006, 2009, 201, 2010, 2013, 2014, 2017-2020, 2025, 2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2006 Krzysztof Kowalczyk <kkowalczyk@gmail.com>
// Copyright (C) 2009 Ilya Gorenbein <igorenbein@finjan.com>
// Copyright (C) 2012 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2013 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
// Copyright (C) 2018, 2019 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2018 Marek Kasik <mkasik@redhat.com>
// Copyright (C) 2024 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <climits>
#include "Object.h"
#include "Array.h"
#include "Dict.h"
#include "Decrypt.h"
#include "Parser.h"
#include "XRef.h"
#include "Error.h"

// Max number of nested objects.  This is used to catch infinite loops
// in the object structure. And also technically valid files with
// lots of nested arrays that made us consume all the stack
constexpr int recursionLimit = 500;

Parser::Parser(XRef *xrefA, std::unique_ptr<Stream> &&streamA, bool allowStreamsA) : lexer { xrefA, std::move(streamA) }
{
    allowStreams = allowStreamsA;
    buf1 = lexer.getObj();
    buf2 = lexer.getObj();
    inlineImg = 0;
}

Parser::Parser(XRef *xrefA, Object *objectA, bool allowStreamsA) : lexer { xrefA, objectA }
{
    allowStreams = allowStreamsA;
    buf1 = lexer.getObj();
    buf2 = lexer.getObj();
    inlineImg = 0;
}

Parser::~Parser() = default;

Object Parser::getObj(int recursion)
{
    return getObj(false, nullptr, cryptRC4, 0, 0, 0, recursion);
}

static std::unique_ptr<GooString> decryptedString(const std::string &s, const unsigned char *fileKey, CryptAlgorithm encAlgorithm, int keyLength, int objNum, int objGen)
{
    DecryptStream decrypt(std::make_unique<MemStream>(s.c_str(), 0, s.size(), Object::null()), fileKey, encAlgorithm, keyLength, { .num = objNum, .gen = objGen });
    if (!decrypt.rewind()) {
        return {};
    }
    std::unique_ptr<GooString> res = std::make_unique<GooString>();
    int c;
    while ((c = decrypt.getChar()) != EOF) {
        res->push_back((char)c);
    }
    return res;
}

Object Parser::getObj(bool simpleOnly, const unsigned char *fileKey, CryptAlgorithm encAlgorithm, int keyLength, int objNum, int objGen, int recursion, bool strict, bool decryptString)
{
    Object obj;

    // refill buffer after inline image data
    if (inlineImg == 2) {
        buf1 = lexer.getObj();
        buf2 = lexer.getObj();
        inlineImg = 0;
    }

    if (unlikely(recursion >= recursionLimit)) {
        return Object::error();
    }

    // array
    if (!simpleOnly && buf1.isCmd("[")) {
        shift();
        obj = Object(std::make_unique<Array>(lexer.getXRef()));
        while (!buf1.isCmd("]") && !buf1.isEOF() && recursion + 1 < recursionLimit) {
            Object obj2 = getObj(false, fileKey, encAlgorithm, keyLength, objNum, objGen, recursion + 1);
            obj.arrayAdd(std::move(obj2));
        }
        if (recursion + 1 >= recursionLimit && strict) {
            goto err;
        }
        if (buf1.isEOF()) {
            error(errSyntaxError, getPos(), "End of file inside array");
            if (strict) {
                goto err;
            }
        }
        shift();