///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup symple
/// @{


#ifndef SCY_Symple_Peer_H
#define SCY_Symple_Peer_H


#include "scy/symple/symple.h"
#include "scy/symple/address.h"
#include "scy/json/json.h"


namespace scy {
namespace smpl {


/// This class represents a peer on the network.
/// A Peer object may also contain arbitrary data set by
/// the client to share with other peers on the network.
/// @see Address for further methods and basic accessors.
class Symple_API Peer : public json::value
{
public:
    Peer();
    Peer(const Peer& r);
    Peer(const json::value& r);
    virtual ~Peer();

    Address address() const;

    std::string id() const;
    std::string user() const;
    std::string name() const;
    // std::string group() const;
    std::string type() const;
    std::string host() const;

    void setID(const std::string& id);
    void setUser(const std::string& user);
    void setName(const std::string& name);
    // void setGroup(const std::string& group);
    void setType(const std::string& type);
    void setHost(const std::string& host);

    /// Updates the peer from the given data object.
    // virtual void update(const json::value& data, bool whiny = false);

    virtual bool valid();

    void print(std::ostream& os) const;

    friend std::ostream& operator<<(std::ostream& os, const Peer& peer)
    {
        peer.print(os);
        return os;
    }

    virtual const char* className() const { return "smpl::Peer"; }
};


} // namespace smpl
} // namespace scy


#endif // SCY_Symple_Peer_H


/// @\}
