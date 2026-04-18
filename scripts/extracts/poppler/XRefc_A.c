// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/XRef.cc
// Segment 1/13

//========================================================================
//
// XRef.cc
//
// Copyright 1996-2003, 2025 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005 Dan Sheridan <dan.sheridan@postman.org.uk>
// Copyright (C) 2005 Brad Hards <bradh@frogmouth.net>
// Copyright (C) 2006, 2008, 2010, 2012-2014, 2016-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2007-2008 Julien Rebetez <julienr@svn.gnome.org>
// Copyright (C) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2009, 2010 Ilya Gorenbein <igorenbein@finjan.com>
// Copyright (C) 2010 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2012, 2013, 2016 Thomas Freitag <Thomas.Freitag@kabelmail.de>
// Copyright (C) 2012, 2013 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright (C) 2013, 2014, 2017, 2019 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013 Pino Toscano <pino@kde.org>
// Copyright (C) 2016 Jakub Alba <jakubalba@gmail.com>
// Copyright (C) 2018, 2019 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2018 Tobias Deiminger <haxtibal@posteo.de>
// Copyright (C) 2019 LE GARREC Vincent <legarrec.vincent@gmail.com>
// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
// Copyright (C) 2010 William Bader <william@newspapersystems.com>
// Copyright (C) 2021 Mahmoud Khalil <mahmoudkhalil11@gmail.com>
// Copyright (C) 2021 Georgiy Sgibnev <georgiy@sgibnev.com>. Work sponsored by lab50.net.
// Copyright (C) 2023, 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2023 Ilaï Deutel <idtl@google.com>
// Copyright (C) 2023 Even Rouault <even.rouault@spatialys.com>
// Copyright (C) 2024 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright (C) 2024 Vincent Lefevre <vincent@vinc17.net>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
// Copyright (C) 2026 Ojas Maheshwari <workonlyojas@gmail.com>
// Copyright (C) 2026 Adam Sampson <ats@offog.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <cctype>
#include <climits>
#include <limits>
#include "goo/gfile.h"
#include "goo/gmem.h"
#include "Object.h"
#include "Stream.h"
#include "Lexer.h"
#include "Parser.h"
#include "Dict.h"
#include "Error.h"
#include "ErrorCodes.h"
#include "XRef.h"

//------------------------------------------------------------------------
// Permission bits
// Note that the PDF spec uses 1 base (eg bit 3 is 1<<2)
//------------------------------------------------------------------------

constexpr int permPrint = 1 << 2; // bit 3
constexpr int permChange = 1 << 3; // bit 4
constexpr int permCopy = 1 << 4; // bit 5
constexpr int permNotes = 1 << 5; // bit 6
constexpr int permFillForm = 1 << 8; // bit 9
constexpr int permAccessibility = 1 << 9; // bit 10
constexpr int permAssemble = 1 << 10; // bit 11
constexpr int permHighResPrint = 1 << 11; // bit 12
constexpr int defPermFlags = 0xfffc;

//------------------------------------------------------------------------
// ObjectStream
//------------------------------------------------------------------------

class ObjectStream
{
public:
    // Create an object stream, using object number <objStrNum>,
    // generation 0.
    ObjectStream(XRef *xref, int objStrNumA, int recursion = 0);

    bool isOk() const { return ok; }

    ~ObjectStream();

    ObjectStream(const ObjectStream &) = delete;
    ObjectStream &operator=(const ObjectStream &) = delete;

    // Return the object number of this object stream.
    int getObjStrNum() const { return objStrNum; }

    // Get the <objIdx>th object from this stream, which should be
    // object number <objNum>, generation 0.
    Object getObject(int objIdx, int objNum);

private:
    int objStrNum; // object number of the object stream
    int nObjects; // number of objects in the stream
    Object *objs; // the objects (length = nObjects)
    int *objNums; // the object numbers (length = nObjects)
    bool ok;
};

ObjectStream::ObjectStream(XRef *xref, int objStrNumA, int recursion)
{
    Parser *parser;
    Goffset *offsets;
    Object objStr, obj1;
    Goffset first;
    int i;

    objStrNum = objStrNumA;
    nObjects = 0;
    objs = nullptr;
    objNums = nullptr;
    ok = false;

    objStr = xref->fetch(objStrNum, 0, recursion);
    if (!objStr.isStream()) {
        return;
    }