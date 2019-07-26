///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup net
/// @{


#ifndef SCY_Net_Address_H
#define SCY_Net_Address_H


#include "scy/net/net.h"
#include <string>
#include <memory>


namespace scy {
namespace net {


/// This class represents an internet (IP) endpoint/socket
/// address. The address can belong either to the
/// IPv4 or the IPv6 address family and consists of a
/// host address and a port number.
class Net_API AddressBase;
class Net_API Address
{
public:
    /// Possible address families for IP addresses.
    enum Family
    {
        IPv4,
        IPv6
    };

    /// Creates a wildcard (all zero) IPv4 Address.
    Address();

    /// Creates a Address from an IP address and a port number.
    ///
    /// The IP address must either be a domain name, or it must
    /// be in dotted decimal (IPv4) or hex string (IPv6) format.
    Address(const std::string& host, uint16_t port);

    /// Creates a Address by copying another one.
    Address(const Address& addr);

    /// Creates a Address from a native socket address.
    Address(const struct sockaddr* addr, socklen_t length);

    /// Creates a Address from an IP address and a
    /// service name or port number.
    ///
    /// The IP address must either be a domain name, or it must
    /// be in dotted decimal (IPv4) or hex string (IPv6) format.
    ///
    /// The given port must either be a decimal port number, or
    /// a service name.
    Address(const std::string& host, const std::string& port);

    /// Creates a Address from an IP address or host name and a
    /// port number/service name. Host name/address and port number must
    /// be separated by a colon. In case of an IPv6 address,
    /// the address part must be enclosed in brackets.
    ///
    /// Examples:
    ///     192.168.1.10:80
    ///     [::ffff:192.168.1.120]:2040
    ///     www.sourcey.com:8080
    explicit Address(const std::string& hostAndPort);

    /// Destroys the Address.
    ~Address();

    /// Assigns another Address.
    Address& operator=(const Address& addr);

    /// Swaps the Address with another one.
    void swap(Address& addr);

    /// Returns the host IP address.
    std::string host() const;

    /// Returns the port number.
    uint16_t port() const;

    /// Returns the length of the internal native socket address.
    socklen_t length() const;

    /// Returns a pointer to the internal native socket address.
    const struct sockaddr* addr() const;

    /// Returns the address family (AF_INET or AF_INET6) of the address.
    int af() const;

    /// Returns a string representation of the address.
    std::string toString() const;

    /// Returns the address family of the host's address.
    Address::Family family() const;

    /// Returns true when the port is set and the address is valid
    /// ie. not wildcard.
    bool valid() const;

    static uint16_t resolveService(const std::string& service);

    static bool validateIP(const std::string& address);

    bool operator<(const Address& addr) const;
    bool operator==(const Address& addr) const;
    bool operator!=(const Address& addr) const;

    friend std::ostream& operator<<(std::ostream& stream, const Address& addr)
    {
        stream << addr.toString();
        return stream;
    }

protected:
    void init(const std::string& host, uint16_t port);

private:
    std::shared_ptr<AddressBase> _base;
};


} // namespace net
} // namespace scy


#endif // SCY_Net_Address_H


/// @\}
