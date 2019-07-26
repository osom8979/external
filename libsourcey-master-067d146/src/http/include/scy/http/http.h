///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup http
/// @{


#ifndef SCY_HTTP_h
#define SCY_HTTP_h


#include "scy/base.h"


// Shared library exports
#if defined(SCY_WIN) && defined(SCY_SHARED_LIBRARY)
    #if defined(HTTP_EXPORTS)
        #define HTTP_API __declspec(dllexport)
    #else
        #define HTTP_API __declspec(dllimport)
    #endif
#else
    #define HTTP_API // nothing
#endif


#endif // SCY_HTTP_H


/// @\}
