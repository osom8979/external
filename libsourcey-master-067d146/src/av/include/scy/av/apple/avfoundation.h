///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
/// @addtogroup av
/// @{


#ifndef SCY_AV_AVFoundation_H
#define SCY_AV_AVFoundation_H


#include "scy/av/devicemanager.h"
#include "scy/base.h"


#ifndef SCY_APPLE
#error "This file is only meant to be compiled for Apple targets"
#endif


namespace scy {
namespace av {
namespace avfoundation {


bool getDeviceList(Device::Type type, std::vector<av::Device>& devices);


} // namespace avfoundation
} // namespace av
} // namespace scy


#endif // SCY_AV_AVFoundation_H
