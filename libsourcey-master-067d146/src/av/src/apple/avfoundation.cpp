///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
/// @addtogroup av
/// @{


#include "scy/logger.h"
#include <ostream>
#include "scy/av/apple/avfoundation.h"


using std::endl;


namespace scy {
namespace av {
namespace avfoundation {


extern bool GetAVFoundationVideoDevices(Device::Type type, std::vector<Device>* devices);


bool getDeviceList(Device::Type type, std::vector<av::Device>& devices)
{
    switch (type) {
        case Device::VideoInput:
        case Device::AudioInput:
            return GetAVFoundationVideoDevices(type, &devices);
        default:
            LWarn("AVFoundation cannot enumerate output devices: Not implemented")
            break;
    }

    return false;
}


} // namespace avfoundation
} // namespace av
} // namespace scy


/// @\}
