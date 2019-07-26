///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup base
/// @{


#ifndef SCY_PacketSignal_H
#define SCY_PacketSignal_H


#include "scy/packet.h"
#include "scy/signal.h"
#include <cstdint>


namespace scy {


/// Signal that broadcasts `IPacket` types.
typedef Signal<void(IPacket&)> PacketSignal;


/// Signal slot that allows listeners to filter polymorphic `IPacket` types.
template <class Class, class RT, class PT, class IT = IPacket>
std::shared_ptr<internal::Slot<RT, IT&>>
packetSlot(Class* instance, RT (Class::*method)(PT&), int id = -1, int priority = -1)
{
    return std::make_shared<internal::Slot<RT, IT&>>(
        new PolymorphicDelegate<Class, RT, PT, IT>(instance, method), instance, id, priority);
}


} // namespace scy


#endif // SCY_PacketSignal_H


/// @\}
