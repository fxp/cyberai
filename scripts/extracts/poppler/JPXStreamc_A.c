// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/JPXStream.cc
// Segment 1/31

//========================================================================
//
// JPXStream.cc
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2008, 2012, 2021, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2012 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2012 Even Rouault <even.rouault@mines-paris.org>
// Copyright (C) 2019 Robert Niemi <robert.den.klurige@gmail.com>
// Copyright (C) 2024, 2025 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include "goo/gmem.h"
#include "Error.h"
#include "JArithmeticDecoder.h"
#include "JPXStream.h"

//~ to do:
//  - precincts
//  - ROI
//  - progression order changes
//  - packed packet headers
//  - support for palettes, channel maps, etc.
//  - make sure all needed JP2/JPX subboxes are parsed (readBoxes)
//  - can we assume that QCC segments must come after the QCD segment?
//  - handle tilePartToEOC in readTilePartData
//  - progression orders 2, 3, and 4
//  - in coefficient decoding (readCodeBlockData):
//    - selective arithmetic coding bypass
//      (this also affects reading the cb->dataLen array)
//    - coeffs longer than 31 bits (should just ignore the extra bits?)
//  - handle boxes larger than 2^32 bytes
//  - the fixed-point arithmetic won't handle 16-bit pixels

//------------------------------------------------------------------------

// number of contexts for the arithmetic decoder
#define jpxNContexts 19

#define jpxContextSigProp 0 // 0 - 8: significance prop and cleanup
#define jpxContextSign 9 // 9 - 13: sign
#define jpxContextMagRef 14 // 14 -16: magnitude refinement
#define jpxContextRunLength 17 // cleanup: run length
#define jpxContextUniform 18 // cleanup: first signif coeff

//------------------------------------------------------------------------

#define jpxPassSigProp 0
#define jpxPassMagRef 1
#define jpxPassCleanup 2

//------------------------------------------------------------------------