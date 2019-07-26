///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup av
/// @{


#ifndef SCY_AV_H
#define SCY_AV_H


#include "scy/base.h"


#define MAX_VIDEO_PACKET_SIZE (3 * 1024 * 1024)
#define MAX_AUDIO_PACKET_SIZE (128 * 1500) ///< 1 second of 48khz 32bit audio
#define MAX_ENCODE_DURATION (3 * 60 * 60 * 1000) ///< 3 hours


#if defined(_WIN32) && !defined(INT64_C) && !defined(__MINGW32__)
#include "inttypes.h"
#endif


// Defined in stdint.h
#ifndef INT64_C
#define INT64_C(c) (c##LL)
#endif
#ifndef UINT64_C
#define UINT64_C(c) (c##ULL)
#endif


// Shared library exports
#if defined(SCY_WIN) && defined(SCY_SHARED_LIBRARY)
    #if defined(AV_EXPORTS)
        #define AV_API __declspec(dllexport)
    #else
        #define AV_API __declspec(dllimport)
    #endif
#else
    #define AV_API // nothing
#endif


#endif // SCY_AV_H


/// @\}
