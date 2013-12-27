/*
  This file is part of the kcal library.

  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/
#ifndef KCALUTILS_EXPORT_H
#define KCALUTILS_EXPORT_H

#include <kdemacros.h>

#ifndef KCALUTILS_EXPORT
# if defined(KDEPIM_STATIC_LIBS)
/* No export/import for static libraries */
#  define KCALUTILS_EXPORT
# elif defined(MAKE_KCALUTILS_LIB)
/* We are building this library */
#  define KCALUTILS_EXPORT KDE_EXPORT
# else
/* We are using this library */
#  define KCALUTILS_EXPORT KDE_IMPORT
# endif
#endif

# ifndef KCALUTILS_EXPORT_DEPRECATED
#  if !defined( WANT_DEPRECATED_KCALUTILS_API )
#    define KCALUTILS_EXPORT_DEPRECATED KDE_DEPRECATED KCALUTILS_EXPORT
#  else
#    define KCALUTILS_EXPORT_DEPRECATED KCALUTILS_EXPORT
#  endif
# endif

#ifdef COMPILING_TESTS
#ifndef KCALUTILS_TEST_EXPORT
# if defined(KDEPIM_STATIC_LIBS)
/* No export/import for static libraries */
#  define KCALUTILS_TEST_EXPORT
# elif defined(MAKE_KCALUTILS_TEST_LIB)
/* We are building this library */
#  define KCALUTILS_TEST_EXPORT KDE_EXPORT
# else
/* We are using this library */
#  define KCALUTILS_TEST_EXPORT KDE_IMPORT
# endif
#endif
#endif /* COMPILING_TESTS */

/**
  @namespace KCalUtils

  @brief
  Contains all the KCalUtils library global classes, objects, and functions.
*/

#endif
