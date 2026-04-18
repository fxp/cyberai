// Source: /Users/xiaopingfeng/Projects/cyberai/targets/poppler-26.04.0/poppler/PDFDoc.cc
// Segment 1/19

//========================================================================
//
// PDFDoc.cc
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
// Copyright (C) 2005, 2006, 2008 Brad Hards <bradh@frogmouth.net>
// Copyright (C) 2005, 2007-2009, 2011-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2008 Julien Rebetez <julienr@svn.gnome.org>
// Copyright (C) 2008, 2010 Pino Toscano <pino@kde.org>
// Copyright (C) 2008, 2010, 2011 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2009 Eric Toombs <ewtoombs@uwaterloo.ca>
// Copyright (C) 2009 Kovid Goyal <kovid@kovidgoyal.net>
// Copyright (C) 2009, 2011 Axel Struebing <axel.struebing@freenet.de>
// Copyright (C) 2010-2012, 2014 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2010 Jakub Wilk <jwilk@jwilk.net>
// Copyright (C) 2010 Ilya Gorenbein <igorenbein@finjan.com>
// Copyright (C) 2010 Srinivas Adicherla <srinivas.adicherla@geodesic.com>
// Copyright (C) 2010 Philip Lorenz <lorenzph+freedesktop@gmail.com>
// Copyright (C) 2011-2016 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2012, 2013 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright (C) 2013, 2014, 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013, 2018 Adam Reichold <adamreichold@myopera.com>
// Copyright (C) 2014 Bogdan Cristea <cristeab@gmail.com>
// Copyright (C) 2015 Li Junling <lijunling@sina.com>
// Copyright (C) 2015 André Guerreiro <aguerreiro1985@gmail.com>
// Copyright (C) 2015 André Esser <bepandre@hotmail.com>
// Copyright (C) 2016, 2020 Jakub Alba <jakubalba@gmail.com>
// Copyright (C) 2017 Jean Ghali <jghali@libertysurf.fr>
// Copyright (C) 2017 Fredrik Fornwall <fredrik@fornwall.net>
// Copyright (C) 2018 Ben Timby <btimby@gmail.com>
// Copyright (C) 2018 Evangelos Foutras <evangelos@foutrelis.com>
// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
// Copyright (C) 2018 Evangelos Rigas <erigas@rnd2.org>
// Copyright (C) 2018 Philipp Knechtges <philipp-dev@knechtges.com>
// Copyright (C) 2019 Christian Persch <chpe@src.gnome.org>
// Copyright (C) 2020 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright (C) 2020 Thorsten Behrens <Thorsten.Behrens@CIB.de>
// Copyright (C) 2020, 2026 Adam Sampson <ats@offog.org>
// Copyright (C) 2021-2024 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2021 Mahmoud Khalil <mahmoudkhalil11@gmail.com>
// Copyright (C) 2021 RM <rm+git@arcsin.org>
// Copyright (C) 2021 Georgiy Sgibnev <georgiy@sgibnev.com>. Work sponsored by lab50.net.
// Copyright (C) 2021-2022 Marek Kasik <mkasik@redhat.com>
// Copyright (C) 2022 Felix Jung <fxjung@posteo.de>
// Copyright (C) 2022 crt <chluo@cse.cuhk.edu.hk>
// Copyright (C) 2022 Erich E. Hoover <erich.e.hoover@gmail.com>
// Copyright (C) 2023-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2024 Vincent Lefevre <vincent@vinc17.net>
// Copyright (C) 2024 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
// Copyright (C) 2025 Juraj Šarinay <juraj@sarinay.com>
// Copyright (C) 2025 Jonathan Hähne <jonathan.haehne@hotmail.com>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>
#include <poppler-config.h>

#include <array>
#include <cctype>
#include <cstdio>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <regex>
#include <sstream>
#include <sys/stat.h>
#include "CryptoSignBackend.h"
#include "goo/GooString.h"
#include "goo/gfile.h"
#include "goo/glibc.h"
#include "GlobalParams.h"
#include "Page.h"
#include "Catalog.h"
#include "Stream.h"
#include "XRef.h"
#include "Linearization.h"
#include "Link.h"
#include "OutputDev.h"
#include "Error.h"
#include "Lexer.h"
#include "SecurityHandler.h"
#include "Decrypt.h"
#include "Outline.h"
#include "PDFDoc.h"
#include "Hints.h"
#include "UTF.h"
#include "FlateEncoder.h"
#include "JSInfo.h"
#include "ImageEmbeddingUtils.h"

//------------------------------------------------------------------------

struct FILECloser
{
    void operator()(FILE *f) { fclose(f); }
};

//------------------------------------------------------------------------