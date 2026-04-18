// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/GfxFont.cc
// Segment 1/20

//========================================================================
//
// GfxFont.cc
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
// Copyright (C) 2005, 2006, 2008-2010, 2012, 2014, 2015, 2017-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2005, 2006 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2006 Takashi Iwai <tiwai@suse.de>
// Copyright (C) 2007 Julien Rebetez <julienr@svn.gnome.org>
// Copyright (C) 2007 Jeff Muizelaar <jeff@infidigm.net>
// Copyright (C) 2007 Koji Otani <sho@bbr.jp>
// Copyright (C) 2007 Ed Catmur <ed@catmur.co.uk>
// Copyright (C) 2008 Jonathan Kew <jonathan_kew@sil.org>
// Copyright (C) 2008 Ed Avis <eda@waniasset.com>
// Copyright (C) 2008, 2010 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2009 Peter Kerzum <kerzum@yandex-team.ru>
// Copyright (C) 2009, 2010 David Benjamin <davidben@mit.edu>
// Copyright (C) 2011 Axel Strübing <axel.struebing@freenet.de>
// Copyright (C) 2011, 2012, 2014 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2012 Yi Yang <ahyangyi@gmail.com>
// Copyright (C) 2012 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2012, 2017 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2013-2016, 2018 Jason Crain <jason@aquaticape.us>
// Copyright (C) 2014 Olly Betts <olly@survex.com>
// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2019 LE GARREC Vincent <legarrec.vincent@gmail.com>
// Copyright (C) 2021, 2022, 2024 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2023 Khaled Hosny <khaled@aliftype.com>
// Copyright (C) 2024 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2024 Vincent Lefevre <vincent@vinc17.net>
// Copyright (C) 2024 G B <glen.browman@veeva.com>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
// Copyright (C) 2026 Adam Sampson <ats@offog.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <climits>
#include <algorithm>
#include "goo/gmem.h"
#include "Error.h"
#include "Object.h"
#include "Dict.h"
#include "GlobalParams.h"
#include "CMap.h"
#include "CharCodeToUnicode.h"
#include "FontEncodingTables.h"
#include "BuiltinFont.h"
#include "UnicodeTypeTable.h"
#include <fofi/FoFiIdentifier.h>
#include <fofi/FoFiType1.h>
#include <fofi/FoFiType1C.h>
#include <fofi/FoFiTrueType.h>
#include "GfxFont.h"
#include "PSOutputDev.h"

//------------------------------------------------------------------------

struct Base14FontMapEntry
{
    const char *altName;
    const char *base14Name;
};