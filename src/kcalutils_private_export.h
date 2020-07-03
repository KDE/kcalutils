/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KCALUTILSPRIVATE_EXPORT_H
#define KCALUTILSPRIVATE_EXPORT_H

#include "kcalutils_export.h"

/* Classes which are exported only for unit tests */
#ifdef BUILD_TESTING
# ifndef KCALUTILS_TESTS_EXPORT
#  define KCALUTILS_TESTS_EXPORT KCALUTILS_EXPORT
# endif
#else /* not compiling tests */
# define KCALUTILS_TESTS_EXPORT
#endif

#endif
