// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/Form.cc
// Segment 1/24

//========================================================================
//
// Form.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2006-2008 Julien Rebetez <julienr@svn.gnome.org>
// Copyright 2007-2012, 2015-2026 Albert Astals Cid <aacid@kde.org>
// Copyright 2007-2008, 2011 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright 2007, 2013, 2016, 2019, 2022 Adrian Johnson <ajohnson@redneon.com>
// Copyright 2007 Iñigo Martínez <inigomartinez@gmail.com>
// Copyright 2008, 2011 Pino Toscano <pino@kde.org>
// Copyright 2008 Michael Vrable <mvrable@cs.ucsd.edu>
// Copyright 2009 Matthias Drochner <M.Drochner@fz-juelich.de>
// Copyright 2009 KDAB via Guillermo Amaral <gamaral@amaral.com.mx>
// Copyright 2010, 2012 Mark Riedesel <mark@klowner.com>
// Copyright 2012 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright 2015 André Guerreiro <aguerreiro1985@gmail.com>
// Copyright 2015 André Esser <bepandre@hotmail.com>
// Copyright 2017 Hans-Ulrich Jüttner <huj@froreich-bioscientia.de>
// Copyright 2017 Bernd Kuhls <berndkuhls@hotmail.com>
// Copyright 2018 Andre Heinecke <aheinecke@intevation.de>
// Copyright 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
// Copyright 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@protonmail.com>
// Copyright 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright 2018-2022, 2026 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright 2019, 2020 2024, Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright 2019 Tomoyuki Kubota <himajin100000@gmail.com>
// Copyright 2019 João Netto <joaonetto901@gmail.com>
// Copyright 2020-2022 Marek Kasik <mkasik@redhat.com>
// Copyright 2020 Thorsten Behrens <Thorsten.Behrens@CIB.de>
// Copyright 2020, 2023, 2024 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
// Copyright 2021 Georgiy Sgibnev <georgiy@sgibnev.com>. Work sponsored by lab50.net.
// Copyright 2021 Theofilos Intzoglou <int.teo@gmail.com>
// Copyright 2021 Even Rouault <even.rouault@spatialys.com>
// Copyright 2022 Alexander Sulfrian <asulfrian@zedat.fu-berlin.de>
// Copyright 2022, 2024 Erich E. Hoover <erich.e.hoover@gmail.com>
// Copyright 2023-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright 2024 Pratham Gandhi <ppg.1382@gmail.com>
// Copyright 2024 Vincent Lefevre <vincent@vinc17.net>
// Copyright 2025, 2026 Juraj Šarinay <juraj@sarinay.com>
// Copyright 2025 Blair Bonnett <blair.bonnett@gmail.com>
// Copyright (C) 2025 Jonathan Hähne <jonathan.haehne@hotmail.com>
//
//========================================================================

#include <config.h>

#include <array>
#include <set>
#include <limits>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include "goo/ft_utils.h"
#include "goo/gfile.h"
#include "goo/GooString.h"
#include "Error.h"
#include "ErrorCodes.h"
#include "CharCodeToUnicode.h"
#include "Object.h"
#include "Array.h"
#include "Dict.h"
#include "Gfx.h"
#include "GfxFont.h"
#include "GlobalParams.h"
#include "Form.h"
#include "PDFDoc.h"
#include "DateInfo.h"
#include "CryptoSignBackend.h"
#include "SignatureInfo.h"
#include "CertificateInfo.h"
#include "XRef.h"
#include "PDFDocEncoding.h"
#include "Annot.h"
#include "Link.h"
#include "Lexer.h"
#include "CIDFontsWidthsBuilder.h"
#include "UTF.h"

#include "fofi/FoFiTrueType.h"
#include "fofi/FoFiIdentifier.h"

#include <ft2build.h>
#include <variant>
#include FT_FREETYPE_H
#include <unordered_set>

// helper for using std::visit to get a dependent false for static_asserts
// to help get compile errors if one ever extends variants
template<class>
inline constexpr bool always_false_v = false;

// return a newly allocated char* containing an UTF16BE string of size length
std::string pdfDocEncodingToUTF16(const std::string &orig)
{
    // double size, a unicode char takes 2 char, add 2 for the unicode marker
    int length = 2 + 2 * orig.size();
    std::string result;
    result.reserve(length);
    // unicode marker
    result.push_back('\xfe');
    result.push_back('\xff');
    // convert to utf16
    for (int i = 2, j = 0; i < length; i += 2, j++) {
        Unicode u = pdfDocEncoding[(unsigned int)((unsigned char)orig[j])] & 0xffff;
        result.push_back((u >> 8) & 0xff);
        result.push_back(u & 0xff);
    }
    return result;
}

static std::unique_ptr<GooString> convertToUtf16(GooString *pdfDocEncodingString)
{
    std::string tmpStr = pdfDocEncodingToUTF16(pdfDocEncodingString->toStr());
    return std::make_unique<GooString>(tmpStr.c_str() + 2, tmpStr.size() - 2); // Remove the unicode BOM
}