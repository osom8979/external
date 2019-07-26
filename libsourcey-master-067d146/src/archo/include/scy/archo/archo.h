///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup archo
/// @{


#ifndef SCY_Archo_H
#define SCY_Archo_H


#include "scy/base.h"


/// Shared library exports
#if defined(SCY_WIN) && defined(SCY_SHARED_LIBRARY)
    #if defined(Archo_EXPORTS)
        #define Archo_API __declspec(dllexport)
    #else
        #define Archo_API __declspec(dllimport)
    #endif
#else
    #define Archo_API // nothing
#endif


#endif // SCY_Archo_H


/// @\}
